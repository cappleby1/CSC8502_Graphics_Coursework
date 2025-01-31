#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Frustum.h"
#include "../nclgl/Camera.h"
#include "../nclgl/CubeRobot.h"
#include <algorithm>

class Camera;
class Shader;
class HeightMap;
class Mesh;
class SceneNodes;
class MeshMaterial;

class Renderer : public OGLRenderer {
public:
	bool autumn;
	bool freeCam;
	Renderer(Window& parent);
	~Renderer(void);
	void RenderScene() override;
	void UpdateScene(float dt) override;
	void InitialiseCamera();
	void UpdateCameraPos(vector<Vector3> cameraPoints, int& currentPos);
	void ChangeScene();

protected:
	void DrawHeightMap();
	void DrawWater();
	void DrawSkybox();
	void DrawNode(SceneNode* n);
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();



	SceneNode* root;

	Shader* lightShader;
	Shader* reflectShader;
	Shader* skyboxShader;
	Shader* sceneShader;
	Shader* effectShader;

	HeightMap* heightMap;

	Mesh* quad;
	Mesh* cube;
	Mesh* treeMesh;

	MeshMaterial* treeMaterial;

	Light* light;

	Camera* camera;
	vector<Vector3> cameraPoints;

	GLuint cubeMap;
	GLuint waterTex;
	GLuint earthTex;
	GLuint earthBump;

	GLuint cubeMap2;
	GLuint earthTex2;
	GLuint earthBump2;
	vector<GLuint> matTextures;


	float waterRotate;
	float waterCycle;

	Vector3 targetPosition;

	int currentPos;
	int currentFrame = 0;
	int delayFrame = 60;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

	Frustum frameFrustum;
};