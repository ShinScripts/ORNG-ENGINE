#pragma once
#include <memory>
#include "WorldTransform.h"
#include "MeshEntity.h"


class BaseLight {
public:
	BaseLight() = default;
	explicit BaseLight(const glm::fvec3& t_color, const float t_ambient_intensity) : color(t_color), ambient_intensity(t_ambient_intensity) {};

	void SetColor(const float r, const float g, const float b) { color = glm::fvec3(r, g, b); }
	void SetAmbientIntensity(const float intensity) { ambient_intensity = intensity; }
	void SetDiffuseIntensity(const float intensity) { diffuse_intensity = intensity; }

	float GetAmbientIntensity() const { return ambient_intensity; }
	float GetDiffuseIntensity() const { return diffuse_intensity; }
	glm::fvec3 GetColor() const { return color; }

protected:
	bool shadows_enabled = true;
	glm::fvec3 color = glm::fvec3(1.0f, 1.0f, 1.0f);
	float ambient_intensity = 0.2f;
	float diffuse_intensity = 1.0f;
};


class DirectionalLight : public BaseLight {
public:
	DirectionalLight();
	~DirectionalLight() { delete mp_light_transform_matrix; }
	auto GetLightDirection() const { return light_direction; };
	void SetLightDirection(const glm::fvec3& dir);
	auto const* GetTransformMatrixPtr() const { return mp_light_transform_matrix; }
private:
	glm::fmat4* mp_light_transform_matrix = nullptr;
	glm::fvec3 light_direction = glm::fvec3(0.0f, 0.0f, -1.f);
};

struct LightAttenuation {
	float constant = 1.0f;
	float linear = 0.05f;
	float exp = 0.01f;
};


class PointLight : public BaseLight {
public:
	PointLight() = default;
	PointLight(const glm::fvec3& position, const glm::fvec3& t_color) { transform.SetPosition(position.x, position.y, position.z); SetColor(t_color.x, t_color.y, t_color.z); SetAmbientIntensity(0.2f); };

	const LightAttenuation& GetAttentuation() const { return attenuation; }
	const WorldTransform& GetWorldTransform() const { return transform; };
	const MeshEntity* GetMeshVisual() const { return mesh_visual; };
	float GetMaxDistance() const { return max_distance; }

	void SetMeshVisual(MeshEntity* p) { mesh_visual = p; }
	void SetAttenuation(const float constant, const float lin, const float exp) { attenuation.constant = constant; attenuation.linear = lin; attenuation.exp = exp; }
	void SetPosition(const float x, const float y, const float z) { if (mesh_visual != nullptr) { transform.SetPosition(x, y, z); mesh_visual->SetPosition(x, y, z); } };
	void SetMaxDistance(const float d) { max_distance = d; };
protected:
	float max_distance = 48.0f;
	MeshEntity* mesh_visual = nullptr;
	LightAttenuation attenuation;
	WorldTransform transform;
};

class SpotLight : public PointLight {
public:
	SpotLight(const glm::fvec3& dir_vec, const float t_aperture);
	~SpotLight() { delete mp_light_transform_matrix; }
	void SetLightDirection(float i, float j, float k);
	void SetAperture(float angle) { aperture = cosf(glm::radians(angle)); }

	auto GetLightDirection() const { return m_light_direction_vec; }
	auto GetAperture() const { return aperture; }
	auto const& GetTransformMatrix() const { return *mp_light_transform_matrix; }
private:
	glm::fmat4* mp_light_transform_matrix = nullptr;
	glm::fvec3 m_light_direction_vec = glm::fvec3(1, 0, 0);
	float aperture = 0.9396f;
};
