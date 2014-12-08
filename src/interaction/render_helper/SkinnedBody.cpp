#include "SkinnedBody.h"
#include "../../glm/gtx/compatibility.hpp"

ofVec3f aiVector3DToOfVec3f (aiVector3D& aiVec) {
	return ofVec3f(aiVec.x, aiVec.y, aiVec.z);
}

ofQuaternion aiQuaternionToOfQuaternion (const aiQuaternion& aiQuat) {
	return ofQuaternion(aiQuat.x, aiQuat.y, aiQuat.z, aiQuat.w);
}

glm::mat4 aiMatrix4x4ToGlmMat4 (aiMatrix4x4 aiMat) {
	return glm::mat4(
		aiMat.a1,
		aiMat.b1,
		aiMat.c1,
		aiMat.d1,
		aiMat.a2,
		aiMat.b2,
		aiMat.c2,
		aiMat.d2,
		aiMat.a3,
		aiMat.b3,
		aiMat.c3,
		aiMat.d3,
		aiMat.a4,
		aiMat.b4,
		aiMat.c4,
		aiMat.d4
	);
}

ofMatrix4x4 aiMatrix4x4ToOfMatrix4x4(const aiMatrix4x4& aim){
	return ofMatrix4x4(
		aim.a1,aim.a2,aim.a3,aim.a4,
        aim.b1,aim.b2,aim.b3,aim.b4,
        aim.c1,aim.c2,aim.c3,aim.c4,
        aim.d1,aim.d2,aim.d3,aim.d4
	);
}

void SkinnedBody::load(const string modelLocation) {
	ofxAssimpModelLoader model;
	model.loadModel("body/human_body.dae");

	vector<vector<int>> boneIndicesAccumulators;
	vector<vector<float>> boneWeightsAccumulators;

	const aiScene *scene = model.getAssimpScene();
	aiMesh* mesh = scene->mMeshes[0];

	//assume only one animation for now...
	
	for (int i = 0; i < mesh->mNumVertices; ++i) {
		vertices.push_back(aiVector3DToOfVec3f(mesh->mVertices[i]));
		normals.push_back(aiVector3DToOfVec3f(mesh->mNormals[i]).normalize());

		//create storage for bone indices and bone weights
		vector<int> boneIndicesAccumulator;
		boneIndicesAccumulators.push_back(boneIndicesAccumulator);
		vector<float> boneWeightsAccumulator;
		boneWeightsAccumulators.push_back(boneWeightsAccumulator);

	}

	//faces from assimp will always be triangular
	for (int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace& face = mesh->mFaces[i];
		for (int j = 0; j < 3; ++j) {
			indices.push_back(face.mIndices[j]);
			//bodyIndices.push_back(face.mIndices[(j + 1) % 3]);
		}
	}

	for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
		aiBone* aBone = mesh->mBones[boneIndex];

		Bone bone;
		bone.name = aBone->mName.data;

		boneNameToIndex[bone.name] = boneIndex;

		bone.inverseBindPoseMatrix = aiMatrix4x4ToGlmMat4(aBone->mOffsetMatrix);

		for (int j = 0; j < aBone->mNumWeights; ++j) {
			aiVertexWeight& weight = aBone->mWeights[j];

			boneIndicesAccumulators[weight.mVertexId].push_back(boneIndex);
			boneWeightsAccumulators[weight.mVertexId].push_back(weight.mWeight);
		}

		bones.push_back(bone);
	}

	for (int i = 0; i < bones.size(); ++i) {
		aiNode* node = scene->mRootNode->FindNode(bones[i].name);

		if (node->mParent == NULL || boneNameToIndex.count(node->mParent->mName.data) == 0) {
			bones[i].parentIndex = -1;
		} else {
			bones[i].parentIndex = boneNameToIndex[node->mParent->mName.data];
		}
	}

	//flatten bone indices and weights for use in attribute
	for (int i = 0; i < vertices.size(); ++i) {

		ofVec4f boneIndicesVector;
		ofVec4f boneWeightsVector;

		for (int j = 0; j < 4; ++j) {
			if (j < boneIndicesAccumulators[i].size()) {
				boneIndicesVector[j] = boneIndicesAccumulators[i][j];
			} else {
				boneIndicesVector[j] = 0;
			}
			if (j < boneWeightsAccumulators[i].size()) {
				boneWeightsVector[j] = boneWeightsAccumulators[i][j];
			} else {
				boneWeightsVector[j] = 0;
			}
		}

		boneIndices.push_back(boneIndicesVector);
		boneWeights.push_back(boneWeightsVector);
	}

	for (int i = 0; i < bones.size(); ++i) {
		aiNode* node = scene->mRootNode->FindNode(bones[i].name);
		bones[i].relativeRotation = glm::quat(aiMatrix4x4ToGlmMat4(node->mTransformation));
		bones[i].relativePosition = glm::vec3(aiMatrix4x4ToGlmMat4(node->mTransformation)[3]);
	}

	for (int i = 0; i < scene->mAnimations[0]->mNumChannels; ++i) {
		aiNodeAnim* nodeAnim = scene->mAnimations[0]->mChannels[i];
		Bone& bone = bones[boneNameToIndex[nodeAnim->mNodeName.data]];
		for (int j = 0; j < nodeAnim->mNumPositionKeys; ++j) {
			bone.positionKeys.push_back(PositionKey(nodeAnim->mPositionKeys[j].mTime, glm::vec3(nodeAnim->mPositionKeys[j].mValue.x, nodeAnim->mPositionKeys[j].mValue.y, nodeAnim->mPositionKeys[j].mValue.z)));
		}
		for (int j = 0; j < nodeAnim->mNumRotationKeys; ++j) {
			bone.rotationKeys.push_back(RotationKey(nodeAnim->mRotationKeys[j].mTime, glm::quat(nodeAnim->mRotationKeys[j].mValue.w, nodeAnim->mRotationKeys[j].mValue.x, nodeAnim->mRotationKeys[j].mValue.y, nodeAnim->mRotationKeys[j].mValue.z)));
		}
	}

	for (int i = 0; i < vertices.size(); ++i) {
		for (int j = 0; j < 4; ++j) {
			int boneIndex = boneIndices[i][j];
			float boneWeight = boneWeights[i][j];

			string boneName = bones[boneIndex].name;

			if (boneWeight > 0) {
				if (boneName == "ELBOW_LEFT" || boneName == "HAND_LEFT" || boneName == "HAND_TIP_LEFT" || boneName == "THUMB_LEFT" || boneName == "WRIST_LEFT") {
					leftArmAndHandVertexIndices.push_back(i);
					break;
				}
				if (boneName == "ELBOW_RIGHT" || boneName == "HAND_RIGHT" || boneName == "HAND_TIP_RIGHT" || boneName == "THUMB_RIGHT" || boneName == "WRIST_RIGHT") {
					rightArmAndHandVertexIndices.push_back(i);
					break;
				}
			}
		}
	}
}

void SkinnedBody::updateAnimations(float leftHandAnimationTime, float rightHandAnimationTime) {
	float currentTime = leftHandAnimationTime;

	for (int i = 0; i < bones.size(); ++i) {
		Bone& bone = bones[i];

		float currentTime = 0.0;
		if (bone.name == "HAND_LEFT" || bone.name == "HAND_TIP_LEFT" || bone.name == "THUMB_LEFT") {
			currentTime = leftHandAnimationTime;
		} else if (bone.name == "HAND_RIGHT" || bone.name == "HAND_TIP_RIGHT" || bone.name == "THUMB_RIGHT") {
			currentTime = rightHandAnimationTime;
		}

		glm::vec3 position;
		glm::quat rotation;

		position = bone.relativePosition;
		rotation = bone.relativeRotation;

			
		if (bone.positionKeys.size() > 0) {
			unsigned int currentKeyIndex = 0;
			while (currentKeyIndex < bone.positionKeys.size() - 1) {
				if (currentTime < bone.positionKeys[currentKeyIndex + 1].time) {
					break;
				}
				currentKeyIndex++;
			}

			if (currentKeyIndex == bone.positionKeys.size() - 1) { //clamp to end of animation
				position = bone.positionKeys[bone.positionKeys.size() - 1].position;
			} else {
				unsigned int nextKeyIndex = (currentKeyIndex + 1) % bone.positionKeys.size();

				PositionKey& currentKey = bone.positionKeys[currentKeyIndex];
				PositionKey& nextKey = bone.positionKeys[nextKeyIndex];

				float timeFraction = (currentTime - currentKey.time) / (nextKey.time - currentKey.time);
				position = glm::lerp(currentKey.position, nextKey.position, timeFraction);
			}
		}
			
		if (bone.rotationKeys.size() > 0) {
			unsigned int currentKeyIndex = 0;
			while (currentKeyIndex < bone.rotationKeys.size() - 1) {
				if (currentTime < bone.rotationKeys[currentKeyIndex + 1].time) {
					break;
				}
				currentKeyIndex++;
			}

			if (currentKeyIndex == bone.rotationKeys.size() - 1) {
				rotation = bone.rotationKeys[bone.rotationKeys.size() - 1].rotation;
			} else {
				unsigned int nextKeyIndex = (currentKeyIndex + 1) % bone.rotationKeys.size();

				RotationKey& currentKey = bone.rotationKeys[currentKeyIndex];
				RotationKey& nextKey = bone.rotationKeys[nextKeyIndex];

				float timeFraction = (currentTime - currentKey.time) / (nextKey.time - currentKey.time);
				rotation = glm::lerp(currentKey.rotation, nextKey.rotation, timeFraction);
			}
		}
			
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0), position);
		glm::mat4 rotationMatrix = glm::toMat4(rotation);
		bone.relativeMatrix = translationMatrix * rotationMatrix;

	}
}

void SkinnedBody::updateMatrices () { //animates positions and updates bone matrices from current rotations and animated positions
	boneMatricesArray.clear();

	for (int i = 0; i < bones.size(); ++i) {
		Bone& bone = bones[i];

		bone.absoluteMatrix = bone.relativeMatrix;

		Bone* currentBone = &bone;

		while (currentBone->parentIndex != -1) { //while there's a parent to visit
			currentBone = &bones[currentBone->parentIndex]; //visit parent
			
			bone.absoluteMatrix = currentBone->relativeMatrix * bone.absoluteMatrix;
		}

		bone.fullMatrix = bone.absoluteMatrix * bone.inverseBindPoseMatrix;

		for (int column = 0; column < 4; ++column) {
			for (int row = 0; row < 4; ++row) {
				boneMatricesArray.push_back(bone.fullMatrix[column][row]);
			}
		}

	}
}

glm::vec3 ofVec3fToGlmVec3a (ofVec3f vec) {
	return glm::vec3(vec.x, vec.y, vec.z);
}

ofVec3f glmVec3ToOfVec3f (glm::vec3 vec) {
	return ofVec3f(vec.x, vec.y, vec.z);
}

ofVec3f SkinnedBody::getVertexPosition (int vertexIndex) {
	glm::vec3 position;
	for (int i = 0; i < 4; ++i) {
		int boneIndex = boneIndices[vertexIndex][i];
		float boneWeight = boneWeights[vertexIndex][i];
		position += boneWeight * glm::vec3(bones[boneIndex].fullMatrix * glm::vec4(ofVec3fToGlmVec3a(vertices[vertexIndex]), 1.0));
	}
	return glmVec3ToOfVec3f(position) * 0.011;
}

ofVec3f SkinnedBody::getVertexNormal (int vertexIndex) {
	glm::vec3 normal;
	for (int i = 0; i < 4; ++i) {
		int boneIndex = boneIndices[vertexIndex][i];
		float boneWeight = boneWeights[vertexIndex][i];
		normal += boneWeight * glm::vec3(bones[boneIndex].fullMatrix * glm::vec4(ofVec3fToGlmVec3a(normals[vertexIndex]), 0.0));
	}

	return glmVec3ToOfVec3f(normal).getNormalized();
}

Bone* SkinnedBody::getBoneByName(const string name) {
	return &bones[boneNameToIndex[name]];
}