#include "KinectEngine.h"
#include "../resourcemanager.h"
#include "../constants.h"

#include "../glm/gtc/type_ptr.hpp"

static ShaderResource bodyTriangleShader("body/body.vert", "body/bodytriangles.frag");
static ShaderResource bodyTriangleShaderEnvironment("body/body.vert", "body/bodytrianglesenvironment.frag");
static ShaderResource bodyTriangleShaderEnvironmentLiquid("body/body.vert", "body/bodytrianglesliquid.frag");
static ShaderResource bodyTriangleShaderEvolution("body/body.vert", "body/bodytrianglesevolution.frag");
static ShaderResource bodyParticleShader("body/body.vert", "body/bodyparticles.frag");
static ShaderResource screenQuadShader("Shaders/screenQuad/screenQuad.vert","Shaders/screenQuad/screenQuad.frag");
static ShaderResource bodyPlain("body/body.vert", "body/body_plain.frag");

const float ANIMATION_SPEED = 2.0;

const float BODY_SCALE = 0.011f;

KinectEngine::KinectEngine() {

}
KinectEngine::~KinectEngine(void) {
}

glm::mat4 getBodyModelMatrix (ofVec3f cameraPosition, float bodyRotationAngle) {
	glm::mat4 scaleMatrix(glm::scale(glm::mat4(1.0f), glm::vec3(BODY_SCALE)));
	glm::mat4 firstTranslate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.8f, 0.1f)));
	glm::mat4 rotationMatrix(glm::rotate(glm::mat4(1.0f), -bodyRotationAngle, glm::vec3(1.0, 0.0, 0.0)));
	glm::mat4 secondTranslate(glm::translate(glm::mat4(1.0f), glm::vec3(cameraPosition.x, cameraPosition.y, cameraPosition.z)));
	return secondTranslate * rotationMatrix * firstTranslate * scaleMatrix;
}

void KinectEngine::setup(char * ip, int port) {

	cout << "KinectEngine::setup start" << endl;

	// prepare kinect listener
	this->ip = ip;
	this->port = port;
	networkConsumer.setup(this->ip, this->port);
	// start kinect listener
	#ifndef NO_ANNOYING_CRAP /// it's too annoying when the threads hang on exit while working
	networkConsumer.startThread();
	#endif

	handsState[0] = HandState::Open;
	handsState[1] = HandState::Open;

	isBonesScaling = false;

	leftHandAnimationTime = 0.0;
	rightHandAnimationTime = 0.0;

	skinnedBody.load("body/human_body.dae");
	skinnedBody.updateAnimations(0.0, 0.0);
	skinnedBody.updateMatrices();

	setupForRendering();
	
	cout << "KinectEngine::setup end" << endl;
}

void KinectEngine::setupForRendering () {
	//setup body vbo (triangles)
	bodyTrianglesVbo.setVertexData(&skinnedBody.vertices[0], skinnedBody.vertices.size(), GL_STATIC_DRAW);
	bodyTrianglesVbo.setNormalData(&skinnedBody.normals[0], skinnedBody.normals.size(), GL_STATIC_DRAW);
	bodyTrianglesVbo.setIndexData(&skinnedBody.indices[0], skinnedBody.indices.size(), GL_STATIC_DRAW);

	//TODO: specify attribute location in shader
	bodyTrianglesVbo.setAttributeData(bodyTriangleShader().getAttributeLocation("boneIndices"), &skinnedBody.boneIndices[0].x, 4, skinnedBody.boneIndices.size(), GL_STATIC_DRAW, sizeof(ofVec4f));
	bodyTrianglesVbo.setAttributeData(bodyTriangleShader().getAttributeLocation("boneWeights"), &skinnedBody.boneWeights[0].x, 4, skinnedBody.boneWeights.size(), GL_STATIC_DRAW, sizeof(ofVec4f));

	//setup body vbo (particles)
	leftBodyParticlesVbo.setVertexData(&skinnedBody.vertices[0], skinnedBody.vertices.size(), GL_STATIC_DRAW);

	//TODO: specify attribute location in shader
	leftBodyParticlesVbo.setAttributeData(bodyParticleShader().getAttributeLocation("boneIndices"), &skinnedBody.boneIndices[0].x, 4, skinnedBody.boneIndices.size(), GL_STATIC_DRAW, sizeof(ofVec4f));
	leftBodyParticlesVbo.setAttributeData(bodyParticleShader().getAttributeLocation("boneWeights"), &skinnedBody.boneWeights[0].x, 4, skinnedBody.boneWeights.size(), GL_STATIC_DRAW, sizeof(ofVec4f));

	//setup body vbo (particles)
	rightBodyParticlesVbo.setVertexData(&skinnedBody.vertices[0], skinnedBody.vertices.size(), GL_STATIC_DRAW);

	//TODO: specify attribute location in shader
	rightBodyParticlesVbo.setAttributeData(bodyParticleShader().getAttributeLocation("boneIndices"), &skinnedBody.boneIndices[0].x, 4, skinnedBody.boneIndices.size(), GL_STATIC_DRAW, sizeof(ofVec4f));
	rightBodyParticlesVbo.setAttributeData(bodyParticleShader().getAttributeLocation("boneWeights"), &skinnedBody.boneWeights[0].x, 4, skinnedBody.boneWeights.size(), GL_STATIC_DRAW, sizeof(ofVec4f));

	vector<float> leftParticleSizes;
	vector<float> rightParticleSizes;

	for (int i = 0; i < skinnedBody.vertices.size(); ++i) {
		if (i % 10 == 0) {
			if (skinnedBody.vertices[i].x > 0) {
				leftParticleSizes.push_back(ofRandom(0.0, 1.0));
				rightParticleSizes.push_back(0.0);
			} else {
				rightParticleSizes.push_back(ofRandom(0.0, 1.0));
				leftParticleSizes.push_back(0.0);
			}
		} else {
			leftParticleSizes.push_back(0.0);
			rightParticleSizes.push_back(0.0);
		}
	}

	leftBodyParticlesVbo.setAttributeData(bodyParticleShader().getAttributeLocation("size"), &leftParticleSizes[0], 1, leftParticleSizes.size(), GL_STATIC_DRAW, sizeof(float));
	rightBodyParticlesVbo.setAttributeData(bodyParticleShader().getAttributeLocation("size"), &rightParticleSizes[0], 1, rightParticleSizes.size(), GL_STATIC_DRAW, sizeof(float));

	environment=new Texture();
	environment->LoadEXR(ofToDataPath("worlds/ultramarine/environment_map.exr"));
}

void KinectEngine::drawBodyBlack(ofVec3f cameraPosition) {
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(10.0, 1.0);

	bodyPlain().begin();
	glUniformMatrix4fv(glGetUniformLocation(bodyPlain().getProgram(), "boneMatrices"), skinnedBody.boneMatricesArray.size() / 16, GL_FALSE, &skinnedBody.boneMatricesArray[0]);
	
	glm::mat4 modelMatrix(getBodyModelMatrix(cameraPosition, 0.0));
	glUniformMatrix4fv(glGetUniformLocation(bodyPlain().getProgram(), "u_modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

	bodyTrianglesVbo.drawElements(GL_TRIANGLES, bodyTrianglesVbo.getNumIndices());
	bodyPlain().end();

	glDisable(GL_POLYGON_OFFSET_FILL);
}

void KinectEngine::drawBodyUltramarine(ofVec3f cameraPosition, float opacity, ofVec3f leftColor, ofVec3f rightColor) {
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	// draw triangles underneath particles
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(10.0, 1.0);

	bodyTriangleShaderEnvironment().begin();

	glm::mat4 modelMatrix(getBodyModelMatrix(cameraPosition, 0.0));
	glUniformMatrix4fv(glGetUniformLocation(bodyTriangleShaderEnvironment().getProgram(), "u_modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

	bodyTriangleShaderEnvironment().setUniform1i("u_environmentMap", 0);
	bodyTriangleShaderEnvironment().setUniform1f("opacity", opacity);

	glActiveTexture(GL_TEXTURE0);
	environment->BindTexture();

	glUniformMatrix4fv(glGetUniformLocation(bodyTriangleShaderEnvironment().getProgram(), "boneMatrices"), skinnedBody.boneMatricesArray.size() / 16, GL_FALSE, &skinnedBody.boneMatricesArray[0]);

	bodyTrianglesVbo.drawElements(GL_TRIANGLES, bodyTrianglesVbo.getNumIndices());
	bodyTriangleShaderEnvironment().end();

	glDisable(GL_POLYGON_OFFSET_FILL);

	glEnable(GL_PROGRAM_POINT_SIZE);

	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
	

	bodyParticleShader().begin();
	glUniformMatrix4fv(glGetUniformLocation(bodyParticleShader().getProgram(), "u_modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

	glUniformMatrix4fv(glGetUniformLocation(bodyParticleShader().getProgram(), "boneMatrices"), skinnedBody.boneMatricesArray.size() / 16, GL_FALSE, &skinnedBody.boneMatricesArray[0]);

	ofVec3f color;
	glDepthMask(GL_FALSE);
	
	color = ofVec3f(10.0, 45.0, 57.0) * 0.5* leftColor * pow(opacity,6);
	bodyParticleShader().setUniform3f("u_color", color.x, color.y, color.z);
	leftBodyParticlesVbo.draw(GL_POINTS, 0, leftBodyParticlesVbo.getNumVertices());
	color = ofVec3f(10.0, 45.0, 57.0) * 0.5* rightColor * pow(opacity,6);
	bodyParticleShader().setUniform3f("u_color", color.x, color.y, color.z);	
	rightBodyParticlesVbo.draw(GL_POINTS, 0, rightBodyParticlesVbo.getNumVertices());

	glDepthMask(GL_TRUE);

	bodyParticleShader().end();

	glDisable(GL_BLEND);
}

void KinectEngine::drawBodyElectric(ofVec3f cameraPosition, float opacity, vector<float>& lightPositions, vector<float>& lightColors, vector<float>& lightAttenuations, float leftEnergy, float rightEnergy) {
	glClear(GL_DEPTH_BUFFER_BIT);
	//glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glDepthMask(GL_TRUE);
	
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//draw triangles underneath particles
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(10.0, 1.0);

	bodyTriangleShader().begin();
	glUniformMatrix4fv(glGetUniformLocation(bodyTriangleShader().getProgram(), "boneMatrices"), skinnedBody.boneMatricesArray.size() / 16, GL_FALSE, &skinnedBody.boneMatricesArray[0]);

	glm::mat4 modelMatrix(getBodyModelMatrix(cameraPosition, 0.0));
	glUniformMatrix4fv(glGetUniformLocation(bodyTriangleShader().getProgram(), "u_modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

	bodyTriangleShader().setUniform1f("opacity", opacity);

	if (lightPositions.size() > 0) {
		glUniform3fv(glGetUniformLocation(bodyTriangleShader().getProgram(), "lightPositions"), lightPositions.size() / 3, &lightPositions[0]);
		glUniform3fv(glGetUniformLocation(bodyTriangleShader().getProgram(), "lightColors"), lightColors.size() / 3, &lightColors[0]);
		glUniform1fv(glGetUniformLocation(bodyTriangleShader().getProgram(), "lightAttenuations"), lightAttenuations.size(), &lightAttenuations[0]);
	}

	bodyTrianglesVbo.drawElements(GL_TRIANGLES, bodyTrianglesVbo.getNumIndices());
	bodyTriangleShader().end();

	glDisable(GL_POLYGON_OFFSET_FILL);

	glEnable(GL_PROGRAM_POINT_SIZE);

	glDepthMask(GL_FALSE);

	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

	bodyParticleShader().begin();
	glUniformMatrix4fv(glGetUniformLocation(bodyParticleShader().getProgram(), "u_modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

	glUniformMatrix4fv(glGetUniformLocation(bodyParticleShader().getProgram(), "boneMatrices"), skinnedBody.boneMatricesArray.size() / 16, GL_FALSE, &skinnedBody.boneMatricesArray[0]);

	ofVec3f leftColor = leftEnergy * 2.0 * ofVec3f(1.0, 2.0, 1.0);
	bodyParticleShader().setUniform3f("u_color", leftColor.x, leftColor.y, leftColor.z);
	leftBodyParticlesVbo.draw(GL_POINTS, 0, leftBodyParticlesVbo.getNumVertices());

	ofVec3f rightColor = rightEnergy * 2.0 * ofVec3f(1.0, 2.0, 1.0);
	bodyParticleShader().setUniform3f("u_color", rightColor.x, rightColor.y, rightColor.z);
	rightBodyParticlesVbo.draw(GL_POINTS, 0, rightBodyParticlesVbo.getNumVertices());
	bodyParticleShader().end();

	glDepthMask(GL_TRUE);
}


void KinectEngine::drawBodyEvolution(ofVec3f cameraPosition, float opacity) {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//draw triangles underneath particles
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(10.0, 1.0);

	bodyTriangleShaderEvolution().begin();
	
	glm::mat4 modelMatrix(getBodyModelMatrix(cameraPosition, 0.0));
	glUniformMatrix4fv(glGetUniformLocation(bodyTriangleShaderEvolution().getProgram(), "u_modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

	bodyTriangleShaderEvolution().setUniform1f("opacity", opacity);
	glUniformMatrix4fv(glGetUniformLocation(bodyTriangleShaderEvolution().getProgram(), "boneMatrices"), skinnedBody.boneMatricesArray.size() / 16, GL_FALSE, &skinnedBody.boneMatricesArray[0]);

	bodyTrianglesVbo.drawElements(GL_TRIANGLES, bodyTrianglesVbo.getNumIndices());
	bodyTriangleShaderEvolution().end();

	glDisable(GL_POLYGON_OFFSET_FILL);

	glEnable(GL_PROGRAM_POINT_SIZE);

	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
	

	bodyParticleShader().begin();

	glUniformMatrix4fv(glGetUniformLocation(bodyParticleShader().getProgram(), "u_modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

	glUniformMatrix4fv(glGetUniformLocation(bodyParticleShader().getProgram(), "boneMatrices"), skinnedBody.boneMatricesArray.size() / 16, GL_FALSE, &skinnedBody.boneMatricesArray[0]);

	bodyParticleShader().setUniform3f("u_color", 10.0*opacity*opacity, 3.0*opacity*opacity, 0.0);

	glDepthMask(GL_FALSE);
	
	leftBodyParticlesVbo.draw(GL_POINTS, 0, leftBodyParticlesVbo.getNumVertices());
	rightBodyParticlesVbo.draw(GL_POINTS, 0, rightBodyParticlesVbo.getNumVertices());

	glDepthMask(GL_TRUE);

	bodyParticleShader().end();
}

void KinectEngine::drawBodyLiquid (ofVec3f cameraPosition, float opacity) {
	
	ofShader &triangle_shader=bodyTriangleShaderEnvironmentLiquid();
	ofShader &particle_shader=bodyParticleShader();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
	
	//draw triangles underneath particles
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(10.0, 1.0);

	triangle_shader.begin();
	glm::mat4 modelMatrix(getBodyModelMatrix(cameraPosition, 0.0));
	glUniformMatrix4fv(glGetUniformLocation(triangle_shader.getProgram(), "u_modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
	glUniformMatrix4fv(glGetUniformLocation(triangle_shader.getProgram(), "boneMatrices"), skinnedBody.boneMatricesArray.size() / 16, GL_FALSE, &skinnedBody.boneMatricesArray[0]);
	
	triangle_shader.setUniform1i("u_environmentMap", 0);
	triangle_shader.setUniform1f("opacity", opacity);

	bodyTrianglesVbo.drawElements(GL_TRIANGLES, bodyTrianglesVbo.getNumIndices());
	triangle_shader.end();

	glDisable(GL_POLYGON_OFFSET_FILL);

	glEnable(GL_PROGRAM_POINT_SIZE);

	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
	

	particle_shader.begin();
	glUniformMatrix4fv(glGetUniformLocation(bodyParticleShader().getProgram(), "u_modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

	glUniformMatrix4fv(glGetUniformLocation(bodyParticleShader().getProgram(), "boneMatrices"), skinnedBody.boneMatricesArray.size() / 16, GL_FALSE, &skinnedBody.boneMatricesArray[0]);
	
	particle_shader.setUniform3f("u_color", 10.0*opacity, 5.0*opacity, 5.0*opacity);

	glDepthMask(GL_FALSE);

	leftBodyParticlesVbo.draw(GL_POINTS, 0, leftBodyParticlesVbo.getNumVertices());
	rightBodyParticlesVbo.draw(GL_POINTS, 0, rightBodyParticlesVbo.getNumVertices());

	glDepthMask(GL_TRUE);

	particle_shader.end();
}

bool KinectEngine::isKinectDetected() {
	bool kinectDetected = false;
	kinectDetected = UserManager::getInstance().hasUsers();

	return kinectDetected;
}

void KinectEngine::update(float dt, float bodyRotationAngle) {
	updateBody(dt, bodyRotationAngle);
}

glm::vec3 ofPointToGlmVec3(ofPoint vec) {
	return glm::vec3(vec.x, vec.y, vec.z);
}

float radiansToDegrees (float radians) {
	return radians * 180.0 / 3.1415926535;
}

float degreesToRadians (float degrees) {
	return (degrees / 180.0) * 3.1415926535;
}

ofVec3f glmVec3ToOfVec3f (const glm::vec3& vec) {
	return ofVec3f(vec.x, vec.y, vec.z);
}

//TODO: pass in rendered body rotation
//body faces negative z, positive x to the right, positive y going up
void KinectEngine::updateBody(float dt, float bodyRotationAngle) {
	static const float COLLINEAR_LIMIT = 0.99;

	skinnedBody.updateAnimations(leftHandAnimationTime, rightHandAnimationTime);

	float rightUpperArmLength, leftUpperArmLength, rightLowerArmLength, leftLowerArmLength;
	// Update body and hands direction
	if (UserManager::getInstance().hasUsers()) {

		UserManager::getInstance().lock();

		std::shared_ptr<Body> user = UserManager::getInstance().getActiveUser();

		glm::mat4 kinectToModel(0, 1, 0, 0, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1); //(x, y, z) -> (y, x, z)

		//correct for body rotation
		kinectToModel = glm::mat4_cast(glm::angleAxis(bodyRotationAngle, glm::vec3(0.0, 1.0, 0.0))) * kinectToModel; //TODO: get this the right way round

		//only rotations are actually applied to the model, extracted from kinect positions, and these rotations take into account the rotational DOFs of each arm joint. this prevents stretching and results in more realistic motion tracking

		//kinect space is the position data from kinect
		//world space is...well...world space
		//model space is relative to the model's main body (the root bone)

		glm::vec3 leftShoulderKinectSpace = ofPointToGlmVec3(user->getJoint(JointType::ShoulderLeft)->getPosition());
		glm::vec3 leftShoulderModelSpace(kinectToModel * glm::vec4(leftShoulderKinectSpace, 1.0));

		glm::vec3 leftElbowKinectSpace = ofPointToGlmVec3(user->getJoint(JointType::ElbowLeft)->getPosition());
		glm::vec3 leftElbowModelSpace(kinectToModel * glm::vec4(leftElbowKinectSpace, 1.0));

		leftUpperArmLength = glm::distance(leftShoulderModelSpace, leftElbowModelSpace);

		glm::vec3 leftWristKinectSpace = ofPointToGlmVec3(user->getJoint(JointType::WristLeft)->getPosition());
		glm::vec3 leftWristModelSpace(kinectToModel * glm::vec4(leftWristKinectSpace, 1.0));

		leftLowerArmLength = glm::distance(leftElbowModelSpace, leftWristModelSpace);

		glm::vec3 leftShoulderElbowModelSpace = glm::normalize(leftElbowModelSpace - leftShoulderModelSpace); //direction from shoulder to elbow
		glm::vec3 leftElbowWristModelSpace = glm::normalize(leftWristModelSpace - leftElbowModelSpace); //direction from elbow to wrist

		glm::vec3 leftShoulderUpModelSpace;
		glm::vec3 leftShoulderRightModelSpace;

		if (glm::dot(leftShoulderElbowModelSpace, leftElbowWristModelSpace) < COLLINEAR_LIMIT) {
			leftShoulderUpModelSpace = glm::normalize(glm::cross(-leftShoulderElbowModelSpace, leftElbowWristModelSpace)); // the up vector for the shoulder rotation
			leftShoulderRightModelSpace = glm::normalize(glm::cross(leftShoulderElbowModelSpace, leftShoulderUpModelSpace));
		} else { //collinear
			leftShoulderRightModelSpace = glm::normalize(glm::cross(leftShoulderElbowModelSpace, glm::vec3(1.0, 0.0, 0.0)));
			leftShoulderUpModelSpace = glm::normalize(glm::cross(leftShoulderRightModelSpace, leftShoulderElbowModelSpace));
		}

		//the positive x axis of the leftShoulder goes along leftShoulderElbowModelSpace
		//the postive y axis of the leftShoulder goes along leftShoulderUpModelSpace
		//the positive z axis of leftShoulder goes along leftShoulderRightModelSpace

		glm::mat4 leftShoulderRelativeRotation(
			leftShoulderElbowModelSpace.x, leftShoulderElbowModelSpace.y, leftShoulderElbowModelSpace.z, 1.0f,
			leftShoulderUpModelSpace.x, leftShoulderUpModelSpace.y, leftShoulderUpModelSpace.z, 1.0f,
			leftShoulderRightModelSpace.x, leftShoulderRightModelSpace.y, leftShoulderRightModelSpace.z, 1.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		skinnedBody.getBoneByName("SHOULDER_LEFT")->relativeRotation = glm::quat(leftShoulderRelativeRotation);

		float leftElbowRotation = radiansToDegrees(acos(glm::dot(leftShoulderElbowModelSpace, leftElbowWristModelSpace))); //elbow is a hinge joint and only has one DOF
		skinnedBody.getBoneByName("ELBOW_LEFT")->relativeRotation = glm::angleAxis((-leftElbowRotation), glm::vec3(0.0, 1.0, 0.0)); //hinge joint, 1DOF


		glm::vec3 rightShoulderKinectSpace = ofPointToGlmVec3(user->getJoint(JointType::ShoulderRight)->getPosition());
		glm::vec3 rightShoulderModelSpace(kinectToModel * glm::vec4(rightShoulderKinectSpace, 1.0));

		glm::vec3 rightElbowKinectSpace = ofPointToGlmVec3(user->getJoint(JointType::ElbowRight)->getPosition());
		glm::vec3 rightElbowModelSpace(kinectToModel * glm::vec4(rightElbowKinectSpace, 1.0));

		glm::vec3 rightWristKinectSpace = ofPointToGlmVec3(user->getJoint(JointType::WristRight)->getPosition());
		glm::vec3 rightWristModelSpace(kinectToModel * glm::vec4(rightWristKinectSpace, 1.0));

		//shoulder have 3 DOF, let's encode the yaw, pitch and roll directly into an orthogonal matrix
		glm::vec3 rightShoulderElbowModelSpace = glm::normalize(rightElbowModelSpace - rightShoulderModelSpace); //direction from shoulder to elbow
		glm::vec3 rightElbowWristModelSpace = glm::normalize(rightWristModelSpace - rightElbowModelSpace); //direction from elbow to wrist

		glm::vec3 rightShoulderUpModelSpace;
		glm::vec3 rightShoulderRightModelSpace;

		if (glm::dot(rightShoulderElbowModelSpace, rightElbowWristModelSpace) < COLLINEAR_LIMIT) {
			rightShoulderUpModelSpace = glm::normalize(glm::cross(-rightShoulderElbowModelSpace, rightElbowWristModelSpace)); // the up vector for the shoulder rotation
			rightShoulderRightModelSpace = glm::normalize(glm::cross(rightShoulderElbowModelSpace, rightShoulderUpModelSpace));
		} else { //collinear
			rightShoulderRightModelSpace = glm::normalize(glm::cross(rightShoulderElbowModelSpace, glm::vec3(-1.0, 0.0, 0.0)));
			rightShoulderUpModelSpace = glm::normalize(glm::cross(rightShoulderRightModelSpace, rightShoulderElbowModelSpace));
		}

		//the positive x axis of the rightShoulder goes along rightShoulderElbowModelSpace
		//the positive y axis of the rightShoulder goes along rightShoulderUpModelSpace
		//the positive z axis of rightShoulder goes along rightShoulderRightModelSpace

		glm::mat4 rightShoulderRelativeRotation(
			rightShoulderElbowModelSpace.x, rightShoulderElbowModelSpace.y, rightShoulderElbowModelSpace.z, 1.0f,
			rightShoulderUpModelSpace.x, rightShoulderUpModelSpace.y, rightShoulderUpModelSpace.z, 1.0f,
			rightShoulderRightModelSpace.x, rightShoulderRightModelSpace.y, rightShoulderRightModelSpace.z, 1.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		skinnedBody.getBoneByName("SHOULDER_RIGHT")->relativeRotation = glm::quat(rightShoulderRelativeRotation);

		float rightElbowRotation = radiansToDegrees(acos(glm::dot(rightShoulderElbowModelSpace, rightElbowWristModelSpace))); //elbow is a hinge joint and only has one DOF
		skinnedBody.getBoneByName("ELBOW_RIGHT")->relativeRotation = glm::angleAxis((-rightElbowRotation), glm::vec3(0.0, 1.0, 0.0)); //hinge joint, 1DOF

		
		//calculate wrist pitch and yaw

		glm::vec3 leftHandKinectSpace = ofPointToGlmVec3(user->getJoint(JointType::HandLeft)->getPosition());

		glm::mat4 leftModelToElbowRotationMatrix = glm::inverse(glm::mat4_cast(skinnedBody.getBoneByName("SHOULDER_LEFT")->relativeRotation) * glm::mat4_cast(skinnedBody.getBoneByName("ELBOW_LEFT")->relativeRotation));

		glm::vec3 leftWristElbowSpace = glm::vec3(leftModelToElbowRotationMatrix * kinectToModel * glm::vec4(leftWristKinectSpace, 1.0));
		glm::vec3 leftHandElbowSpace = glm::vec3(leftModelToElbowRotationMatrix * kinectToModel * glm::vec4(leftHandKinectSpace, 1.0));

		glm::vec3 leftWristHandElbowSpace = glm::normalize(leftHandElbowSpace - leftWristElbowSpace);

		glm::vec3 leftWristRightElbowSpace = glm::normalize(glm::cross(leftWristHandElbowSpace, glm::vec3(0.0, 1.0, 0.0)));
		glm::vec3 leftWristUpElbowSpace = glm::normalize(glm::cross(leftWristRightElbowSpace, leftWristHandElbowSpace));

		glm::mat4 leftWristRelativeRotation(
			leftWristHandElbowSpace.x, leftWristHandElbowSpace.y, leftWristHandElbowSpace.z, 1.0f,
			leftWristUpElbowSpace.x, leftWristUpElbowSpace.y, leftWristUpElbowSpace.z, 1.0f,
			leftWristRightElbowSpace.x, leftWristRightElbowSpace.y, leftWristRightElbowSpace.z, 1.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		//skinnedBody.getBoneByName("WRIST_LEFT")->relativeRotation = glm::quat(leftWristRelativeRotation);
		
		glm::vec3 rightHandKinectSpace = ofPointToGlmVec3(user->getJoint(JointType::HandRight)->getPosition());

		glm::mat4 rightModelToElbowRotationMatrix = glm::inverse(glm::mat4_cast(skinnedBody.getBoneByName("SHOULDER_RIGHT")->relativeRotation) * glm::mat4_cast(skinnedBody.getBoneByName("ELBOW_RIGHT")->relativeRotation));

		glm::vec3 rightWristElbowSpace = glm::vec3(rightModelToElbowRotationMatrix * kinectToModel * glm::vec4(rightWristKinectSpace, 1.0));
		glm::vec3 rightHandElbowSpace = glm::vec3(rightModelToElbowRotationMatrix * kinectToModel * glm::vec4(rightHandKinectSpace, 1.0));

		glm::vec3 rightWristHandElbowSpace = glm::normalize(rightHandElbowSpace - rightWristElbowSpace);

		glm::vec3 rightWristRightElbowSpace = glm::normalize(glm::cross(rightWristHandElbowSpace, glm::vec3(0.0, 1.0, 0.0)));
		glm::vec3 rightWristUpElbowSpace = glm::normalize(glm::cross(rightWristRightElbowSpace, rightWristHandElbowSpace));

		glm::mat4 rightWristRelativeRotation(
			rightWristHandElbowSpace.x, rightWristHandElbowSpace.y, rightWristHandElbowSpace.z, 1.0f,
			rightWristUpElbowSpace.x, rightWristUpElbowSpace.y, rightWristUpElbowSpace.z, 1.0f,
			rightWristRightElbowSpace.x, rightWristRightElbowSpace.y, rightWristRightElbowSpace.z, 1.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		//skinnedBody.getBoneByName("WRIST_RIGHT")->relativeRotation = glm::quat(rightWristRelativeRotation);

		glm::vec3 leftElbowWorldSpace = glm::vec3(skinnedBody.getBoneByName("ELBOW_LEFT")->absoluteMatrix[3]);
		glm::vec3 leftWristWorldSpace = glm::vec3(skinnedBody.getBoneByName("WRIST_LEFT")->absoluteMatrix[3]);
		glm::vec3 leftHandWorldSpace = glm::vec3(skinnedBody.getBoneByName("HAND_LEFT")->absoluteMatrix[3]);
		leftHandDirection = glmVec3ToOfVec3f(glm::normalize(leftWristWorldSpace - leftElbowWorldSpace));
		leftHandPosition = glmVec3ToOfVec3f(leftHandWorldSpace) * BODY_SCALE;

		glm::vec3 rightElbowWorldSpace = glm::vec3(skinnedBody.getBoneByName("ELBOW_RIGHT")->absoluteMatrix[3]);
		glm::vec3 rightWristWorldSpace = glm::vec3(skinnedBody.getBoneByName("WRIST_RIGHT")->absoluteMatrix[3]);
		glm::vec3 rightHandWorldSpace = glm::vec3(skinnedBody.getBoneByName("HAND_RIGHT")->absoluteMatrix[3]);
		rightHandDirection = glmVec3ToOfVec3f(glm::normalize(rightWristWorldSpace - rightElbowWorldSpace));
		rightHandPosition = glmVec3ToOfVec3f(rightHandWorldSpace) * BODY_SCALE;

		UserManager::getInstance().unlock();

	} else {
		skinnedBody.getBoneByName("SHOULDER_LEFT")->relativeRotation = glm::quat(glm::mat4(
			0.0, 0.0, 1.0, 1.0f,
			1.0, 0.0, 0.0, 1.0f,
			0.0, 1.0, 0.0, 1.0f,
			0.0f, 0.0f, 0.0f, 1.0f));

		skinnedBody.getBoneByName("SHOULDER_RIGHT")->relativeRotation = glm::quat(glm::mat4(
			0.0, 0.0, 1.0, 1.0f,
			-1.0, 0.0, 0.0, 1.0f,
			0.0, -1.0, 0.0, 1.0f,
			0.0f, 0.0f, 0.0f, 1.0f));

		skinnedBody.getBoneByName("ELBOW_LEFT")->relativeRotation = glm::angleAxis((0.0f), glm::vec3(0.0, 1.0, 0.0)); //hinge joint, 1DOF
		skinnedBody.getBoneByName("ELBOW_RIGHT")->relativeRotation = glm::angleAxis((0.0f), glm::vec3(0.0, 1.0, 0.0)); //hinge joint, 1DOF

		glm::vec3 leftElbowWorldSpace = glm::vec3(skinnedBody.getBoneByName("ELBOW_LEFT")->absoluteMatrix[3]);
		glm::vec3 leftWristWorldSpace = glm::vec3(skinnedBody.getBoneByName("WRIST_LEFT")->absoluteMatrix[3]);
		leftHandDirection = glmVec3ToOfVec3f(glm::normalize(leftWristWorldSpace - leftElbowWorldSpace));
		leftHandPosition = glmVec3ToOfVec3f(leftWristWorldSpace) * BODY_SCALE;

		glm::vec3 rightElbowWorldSpace = glm::vec3(skinnedBody.getBoneByName("ELBOW_RIGHT")->absoluteMatrix[3]);
		glm::vec3 rightWristWorldSpace = glm::vec3(skinnedBody.getBoneByName("WRIST_RIGHT")->absoluteMatrix[3]);
		rightHandDirection = glmVec3ToOfVec3f(glm::normalize(rightWristWorldSpace - rightElbowWorldSpace));
		rightHandPosition = glmVec3ToOfVec3f(rightWristWorldSpace) * BODY_SCALE;

	}
	skinnedBody.updateMatrices();
}

void KinectEngine::exit() {
    // stop the network consumer thread
	cout << "KinectEngine::exit - stopping network consumer (" << networkConsumer.getThreadId() << ")..." << endl;
	networkConsumer.close();
	networkConsumer.waitForThread(true);
}

SkinnedBody* KinectEngine::getSkinnedBody() {
	return &skinnedBody;
}

//all data from KinectEngine (positions, directions) are returned in body space (origin between feet, y-up, facing -z)
vector<ofPoint> KinectEngine::getHandsPosition() {
	vector<ofPoint> handsPosition = vector<ofPoint>();
	handsPosition.push_back(leftHandPosition);
	handsPosition.push_back(rightHandPosition);
	return handsPosition;
}

vector<ofVec3f> KinectEngine::getHandsDirection() {
	vector<ofVec3f> handsDirection = vector<ofPoint>();
	handsDirection.push_back(leftHandDirection);
	handsDirection.push_back(rightHandDirection);
	return handsDirection;
}

vector<HandState::HandState> KinectEngine::getHandsState() {
	vector<HandState::HandState> handsState = vector<HandState::HandState>();
	if (UserManager::getInstance().hasUsers()) {
		// just use the first user
		UserManager::getInstance().lock();
		std::shared_ptr<Body> user = UserManager::getInstance().getActiveUser();
		handsState.push_back(user->getLeftHandState());
		handsState.push_back(user->getRightHandState());
		UserManager::getInstance().unlock();
	} else {
		// just push to dummy states
		handsState.push_back(HandState::Closed);
		handsState.push_back(HandState::Closed);
	}
	return handsState;
}

bool KinectEngine::isLeftHandOpen() {
	bool isOpen = false;
	if (UserManager::getInstance().hasUsers()) {
		// just use the first user
		UserManager::getInstance().lock();
		std::shared_ptr<Body> user = UserManager::getInstance().getActiveUser();
		isOpen = user->getLeftHandState() == HandState::Open;
		UserManager::getInstance().unlock();
	}
	return isOpen;
}

bool KinectEngine::isRightHandOpen() {
	bool isOpen = false;
	if (UserManager::getInstance().hasUsers()) {
		// just use the first user
		UserManager::getInstance().lock();
		std::shared_ptr<Body> user = UserManager::getInstance().getActiveUser();
		isOpen = user->getRightHandState() == HandState::Open;
		UserManager::getInstance().unlock();
	}
	return isOpen;
}
