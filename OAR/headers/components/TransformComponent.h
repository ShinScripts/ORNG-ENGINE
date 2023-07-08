#pragma once
#include "Component.h"

namespace ORNG {

	class TransformComponent2D {
	public:

		void SetScale(float x, float y);
		void SetOrientation(float rot);
		void SetPosition(float x, float y);

		glm::mat3 GetMatrix() const;
		glm::vec2 GetPosition() const;

	private:
		glm::vec2 m_scale = glm::vec2(1.0f, 1.0f);
		float m_rotation = 0.0f;
		glm::vec2 m_pos = glm::vec2(0.0f, 0.0f);
	};

	class TransformComponent : public Component
	{
	public:
		friend class SceneSerializer;
		friend class EditorLayer;
		TransformComponent(SceneEntity* p_entity = nullptr) : Component(p_entity) {};

		void SetParentTransform(TransformComponent* p_transform) {
			if (mp_parent_transform)
				RemoveParentTransform();

			p_transform->AddChildTransform(this);
		}

		void RemoveParentTransform() {
			mp_parent_transform->RemoveChildTransform(this);
		}

		TransformComponent* GetParentTransform() {
			return mp_parent_transform;
		}


		void SetScale(float scaleX, float scaleY, float scaleZ) {
			glm::vec3 scale{ scaleX, scaleY, scaleZ };
			SetScale(scale);
		}

		void SetAbsolutePosition(glm::vec3 pos) {
			glm::vec3 absolute_pos = GetAbsoluteTransforms()[0];

			SetPosition(pos - (absolute_pos - m_pos));
		}

		void SetOrientation(float x, float y, float z) {
			glm::vec3 orientation{x, y, z};
			SetOrientation(orientation);
		}
		void SetPosition(float x, float y, float z) {
			glm::vec3 pos{x, y, z};
			SetPosition(pos);
		}

		void SetPosition(const glm::vec3 pos) {
			m_pos = pos;
			RebuildMatrix(UpdateType::TRANSLATION);
		};

		void SetScale(const glm::vec3 scale) {
			m_scale = scale;
			RebuildMatrix(UpdateType::SCALE);
		};

		void SetOrientation(const glm::vec3 rot) {
			m_rotation = rot;
			RebuildMatrix(UpdateType::ORIENTATION);
		};

		void SetAbsoluteMode(bool mode) {
			m_is_absolute = mode;
			RebuildMatrix(UpdateType::ALL);
		}

		const glm::mat4x4& GetMatrix() const { return m_transform; };

		// Returns inherited position([0]), scale([1]), rotation ([2]) including this components transforms.
		std::array<glm::vec3, 3> GetAbsoluteTransforms() const;

		glm::vec3 GetPosition() const;
		glm::vec3 GetScale() const { return m_scale; };
		glm::vec3 GetRotation() const { return m_rotation; };

		enum class CallbackType {
			SPOTLIGHT = 0,
			PHYSICS = 1,
			MESH = 2,
			CAMERA = 3
		};

		enum class UpdateType {
			TRANSLATION = 0,
			SCALE = 1,
			ORIENTATION = 2,
			ALL = 3
		};

		// These callbacks will be called whenever the transform changes, given by component systems/managers
		std::unordered_map<CallbackType, std::function<void([[maybe_unused]] UpdateType type)>> update_callbacks;
	private:

		// If true, transform will not take parent transforms into account when building matrix.
		bool m_is_absolute = false;

		void RebuildMatrix(UpdateType type);

		void RemoveChildTransform(TransformComponent* p_transform) {
			auto it = std::find(m_child_transforms.begin(), m_child_transforms.end(), p_transform);
			m_child_transforms.erase(it);
			p_transform->mp_parent_transform = nullptr;
		}

		void AddChildTransform(TransformComponent* p_transform) {
			m_child_transforms.push_back(p_transform);
			p_transform->mp_parent_transform = this;
		}

		glm::mat4 m_transform = glm::mat4(1);

		TransformComponent* mp_parent_transform = nullptr;
		std::vector<TransformComponent*> m_child_transforms;
		glm::vec3 m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 m_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 m_pos = glm::vec3(0.0f, 0.0f, 0.0f);


	};

}