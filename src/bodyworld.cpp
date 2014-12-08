#include "bodyworld.h"
#include "audio.h"

static ShaderResource bodyShader("worlds/body/body.vert", "worlds/body/body.frag");

DEFINE_WORLD(BodyWorld)

BodyWorld::BodyWorld(Engine *engine):World(engine) {
	//ctor
	voicePromptFileName = "";
}
BodyWorld::~BodyWorld() {}

void BodyWorld::setup() {
	World::setup();
	
	bodyVbo.setVertexData(&engine->kinectEngine.getSkinnedBody()->vertices[0], engine->kinectEngine.getSkinnedBody()->vertices.size(), GL_STATIC_DRAW);
	bodyVbo.setNormalData(&engine->kinectEngine.getSkinnedBody()->normals[0], engine->kinectEngine.getSkinnedBody()->normals.size(), GL_STATIC_DRAW);
	bodyVbo.setIndexData(&engine->kinectEngine.getSkinnedBody()->indices[0], engine->kinectEngine.getSkinnedBody()->indices.size(), GL_STATIC_DRAW);

	bodyVbo.setAttributeData(bodyShader().getAttributeLocation("boneIndices"), &engine->kinectEngine.getSkinnedBody()->boneIndices[0].x, 4, engine->kinectEngine.getSkinnedBody()->boneIndices.size(), GL_STATIC_DRAW, sizeof(ofVec4f));
	bodyVbo.setAttributeData(bodyShader().getAttributeLocation("boneWeights"), &engine->kinectEngine.getSkinnedBody()->boneWeights[0].x, 4, engine->kinectEngine.getSkinnedBody()->boneWeights.size(), GL_STATIC_DRAW, sizeof(ofVec4f));

	camera.setupPerspective(false, 90.0, 0.001, 10);
	camera.setPosition(0.0, 1.8, 0.0);
}

void BodyWorld::beginExperience() {
	//PlayAmbientSound("worlds/ultramarine/sounds/Ambience_Bass.ogg", 1.0, true);
	PlayAmbientSound("worlds/electric/sounds/Ambience_Loop.ogg", 1.0, true);
}

void BodyWorld::endExperience() {
}

void BodyWorld::update(float dt){

}

void BodyWorld::keyPressed(int key)
{
	// Start video playback
	if(key == 13)
	{
		PlayAmbientSound("worlds/ultramarine/sounds/Ambience_Bass.ogg", 1.0, true);
	}

	if(key == 'x')
	{
		PlayAmbientSound("worlds/electric/sounds/Ambience_Loop.ogg", 1.0, true);
	}

	World::keyPressed(key);
}

//TODO: custom bloom params
BloomParameters BodyWorld::getBloomParams() {
	return BloomParameters();
}

void BodyWorld::drawScene(renderType render_type, ofFbo *fbo) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);


	bodyShader().begin();
	glUniformMatrix4fv(glGetUniformLocation(bodyShader().getProgram(), "boneMatrices"), engine->kinectEngine.getSkinnedBody()->boneMatricesArray.size() / 16, GL_FALSE, &engine->kinectEngine.getSkinnedBody()->boneMatricesArray[0]);

	bodyVbo.drawElements(GL_TRIANGLES, bodyVbo.getNumIndices());
	bodyShader().end();

	ofLine(engine->kinectEngine.getHandsPosition()[0], engine->kinectEngine.getHandsPosition()[0] + engine->kinectEngine.getHandsDirection()[0]);
	ofLine(engine->kinectEngine.getHandsPosition()[1], engine->kinectEngine.getHandsPosition()[1] + engine->kinectEngine.getHandsDirection()[1]);
}


float BodyWorld::getDuration() {
	return 30.0;
}