#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	autumn = true;
	freeCam = false;
	quad = Mesh::GenerateQuad();
	cube = Mesh::LoadFromMeshFile("OffsetCubeY.msh");
	treeMesh = Mesh::LoadFromMeshFile("tree.msh");
	treeMaterial = new MeshMaterial("tree.mat");

	heightMap = new HeightMap(TEXTUREDIR"forest_noise.png");

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR "water.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR "forest_floor.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR "forest_floor_bump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR "forest_east.jpg", TEXTUREDIR "forest_west.jpg",
		TEXTUREDIR "forest_up.jpg", TEXTUREDIR "forest_down.jpg",
		TEXTUREDIR "forest_south.jpg", TEXTUREDIR "forest_north.jpg",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	earthTex2 = SOIL_load_OGL_texture(TEXTUREDIR "snow.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump2 = SOIL_load_OGL_texture(TEXTUREDIR "snow_bump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap2 = SOIL_load_OGL_cubemap(
		TEXTUREDIR "snow_east.jpg", TEXTUREDIR "snow_west.jpg",
		TEXTUREDIR "snow_up.jpg", TEXTUREDIR "snow_down.jpg",
		TEXTUREDIR "snow_south.jpg", TEXTUREDIR "snow_north.jpg",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!earthTex || !earthBump || !cubeMap || !waterTex) {return;}

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(earthTex2, true);
	SetTextureRepeating(earthBump2, true);
	SetTextureRepeating(waterTex, true);



	reflectShader = new Shader("reflectVertex.glsl", "reflectFragment.glsl");
	skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");
	lightShader = new Shader("PerPixelVertex.glsl", "PerPixelFragment.glsl");
	sceneShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	effectShader = new Shader("SceneVertex.glsl", "effectShader.glsl");

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !lightShader->LoadSuccess()) {return;}

	Vector3 heightmapSize = heightMap->GetHeightmapSize();
	camera = new Camera(-45.0f, 0.0f, heightmapSize * Vector3(0.5f, 5.0f, 0.5f));
	light = new Light(heightmapSize * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), heightmapSize.x);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	root = new SceneNode();


	root->AddChild(new CubeRobot(cube));

	waterRotate = 0.0f;
	waterCycle = 0.0f;
	init = true;
}

Renderer::~Renderer(void) {
	delete camera;
	delete heightMap;
	delete quad;
	delete reflectShader;
	delete skyboxShader;
	delete lightShader;
	delete light;
}


void Renderer::UpdateScene(float dt) {
	if (!freeCam) 
	{
		UpdateCameraPos(cameraPoints, currentPos);
	}
	else
	{
		camera->UpdateCamera(dt);
	}

	viewMatrix = camera->BuildViewMatrix();
	waterRotate += dt * 1.0f;
	waterCycle += dt * 0.1f;
	frameFrustum.FromMatrix(projMatrix * viewMatrix);
	root->Update(dt);
}

void Renderer::RenderScene() {
	BuildNodeLists(root);
	SortNodeLists();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	DrawSkybox();
	DrawHeightMap();
	DrawWater();

	BindShader(sceneShader);
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(),
		"diffuseTex"), 1);
	DrawNode(root);
	DrawNodes();
	ClearNodeLists();

	const Matrix4* invBindPose = treeMesh->GetInverseBindPose();

}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "cubeTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	if (autumn)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
		glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "cubeTex"), 0);
	}
	else
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap2);
		glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "cubeTex"), 0);
	}

	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawHeightMap() {
	BindShader(lightShader);
	SetShaderLight(*light);

	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex"), 0);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	if (autumn)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, earthTex);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex"), 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, earthBump);
	}
	else
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, earthTex2);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex"), 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, earthBump2);
	}

	heightMap->Draw();
}

void Renderer::DrawWater() {
	BindShader(reflectShader);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);

	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	if (autumn)
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	}
	else 
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap2);
	}


	Vector3 hSize = heightMap->GetHeightmapSize();
	modelMatrix =
		Matrix4::Translation(hSize * 0.5f) *
		Matrix4::Scale(hSize * 0.5f) *
		Matrix4::Rotation(90, Vector3(1, 0, 0));

	textureMatrix =
		Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
		Matrix4::Scale(Vector3(10, 10, 10)) *
		Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();
	quad->Draw();
}

void Renderer::InitialiseCamera()
{
	currentPos = 0;
	cameraPoints =
	{
		Vector3(2509.2, 2000, 1496.96),
		Vector3(2035.47, 2000, 1578.45),
		Vector3(1572.27, 2000, 2484.79),
		Vector3(2116.22, 700.023, 2067.31),
	};
}

void Renderer::UpdateCameraPos(vector<Vector3> cameraPoints, int& currentPos) {
	currentFrame++;

	if (currentFrame >= delayFrame) {
		if (!cameraPoints.empty() && currentPos >= 0 && currentPos < cameraPoints.size()) {
			camera->SetPosition(cameraPoints[currentPos]);
			currentPos = (currentPos + 1) % cameraPoints.size();
		}
		currentFrame = 0;  
	}
}

void Renderer::BuildNodeLists(SceneNode* from) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) {
			transparentNodeList.push_back(from);
		}
		else {
			nodeList.push_back(from);
		}
	}
	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); ++i) {
		BuildNodeLists((*i));
	}
}

void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);
	std::sort(nodeList.begin(), nodeList.end(), SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes() {
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList) {
		DrawNode(i);
	}
}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		Matrix4 model = n->GetWorldTransform() *
			Matrix4::Scale(n->GetModelScale());
		glUniformMatrix4fv(
			glGetUniformLocation(sceneShader->GetProgram(),
				"modelMatrix"), 1, false, model.values);
		//Vector4 temp = n->GetColour();
		auto temp = (float*)&n->GetColour();
		float r = temp[0];
		float g = temp[1];
		float b = temp[2];
		float a = temp[3];
		glUniform4fv(glGetUniformLocation(sceneShader->GetProgram(),
			"nodeColour"), 1, (float*)&n->GetColour());
		glUniform1i(glGetUniformLocation(sceneShader->GetProgram(),
			"useTexture"), 0);
		n->Draw(*this);
	}
	for (vector < SceneNode* >::const_iterator
		i = n->GetChildIteratorStart();
		i != n->GetChildIteratorEnd(); ++i) {
		DrawNode(*i);
	}
}


