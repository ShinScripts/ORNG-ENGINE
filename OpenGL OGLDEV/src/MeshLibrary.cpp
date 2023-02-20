#include <glew.h>
#include <iostream>
#include <execution>
#include <glm/gtx/matrix_major_storage.hpp>
#include "MeshLibrary.h"


void MeshLibrary::Init() {
	shaderLibrary.Init();
	grid_mesh.Init();
}

void MeshLibrary::DrawGrid(const ViewData& data) {
	shaderLibrary.grid_shader.ActivateProgram();
	shaderLibrary.grid_shader.SetProjection(glm::colMajor4(data.projectionMatrix));
	shaderLibrary.grid_shader.SetCamera(glm::colMajor4(data.cameraMatrix));
	shaderLibrary.grid_shader.SetCameraPos(data.camera_pos);
	grid_mesh.Draw();
}

void MeshLibrary::RenderLightingShaderMeshes(const ViewData& data) {

	shaderLibrary.lighting_shader.ActivateProgram();
	shaderLibrary.lighting_shader.SetProjection(glm::colMajor4(data.projectionMatrix));
	shaderLibrary.lighting_shader.SetCamera(glm::colMajor4(data.cameraMatrix));

	BaseLight base_light = BaseLight();
	PointLight point_light = PointLight(glm::fvec3(-100.0f, 0.0f, -100.0f), glm::fvec3(0.5f, 0.0f, 0.0f));
	/*if (lightColor.x > 1.0f || lightColor.x < 0.0f) {
		deltaX *= -1.0f;
	}
	if (lightColor.y > 1.0f || lightColor.y < 0.0f) {
		deltaY *= -1.0f;
	}
	if (lightColor.z > 1.0f || lightColor.z < 0.0f) {
		deltaZ *= -1.0f;
	}
	lightColor = glm::fvec3(lightColor.x + deltaX, lightColor.y + deltaY, lightColor.z + deltaZ)*/;
	base_light.color = glm::fvec3(0.2f, 0.2f, 0.2f);
	base_light.ambient_intensity = 1.0f;
	shaderLibrary.lighting_shader.SetTextureUnit(GL_TEXTURE0);
	shaderLibrary.lighting_shader.SetPointLight(point_light);
	shaderLibrary.lighting_shader.SetAmbientLight(base_light);

	for (BasicMesh& mesh : lightingShaderMeshes) {

		mesh.UpdateTransformBuffers(data);
		mesh.Render();
	}
}

void MeshLibrary::RenderAllMeshes(const ViewData& data) {
	RenderLightingShaderMeshes(data);
}
