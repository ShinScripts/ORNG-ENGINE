#pragma once
#include <glm/glm.hpp>
#include "Shader.h"

class GridShader : public Shader {
public:
	void Init() override;
	void ActivateProgram() override;
	const GLint& GetProjectionLocation() const;
	const GLint& GetCameraLocation() const;
	const GLint& GetSamplerLocation() const;
	void SetProjection(const glm::fmat4& proj);
	void SetCameraPos(const glm::fvec3 pos);
	void SetCamera(const glm::fmat4& cam);
private:
	void InitUniforms() override;
	unsigned int vert_shader_id;
	unsigned int frag_shader_id;
	unsigned int programID;
	GLint camera_pos_location;
	GLint projectionLocation;
	GLint cameraLocation;
	GLint samplerLocation;
};