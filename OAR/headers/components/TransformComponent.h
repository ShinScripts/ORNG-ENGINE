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



		void SetScale(float scaleX, float scaleY, float scaleZ) {
			glm::vec3 scale{ scaleX, scaleY, scaleZ };
			SetScale(scale);
		}

		void SetAbsoluteScale(glm::vec3 scale) {
			glm::vec3 abs_scale = GetAbsoluteTransforms()[1];

			SetScale(scale / (abs_scale / m_scale));
		}

		inline void SetAbsolutePosition(glm::vec3 pos) {
			glm::vec3 absolute_pos = GetAbsoluteTransforms()[0];

			SetPosition(pos - (absolute_pos - m_pos));
		}

		inline void SetAbsoluteOrientation(glm::vec3 orientation) {
			glm::vec3 absolute_orientation = GetAbsoluteTransforms()[2];

			SetOrientation(orientation - (absolute_orientation - m_orientation));
		}

		inline void SetOrientation(float x, float y, float z) {
			glm::vec3 orientation{x, y, z};
			SetOrientation(orientation);
		}
		inline void SetPosition(float x, float y, float z) {
			glm::vec3 pos{x, y, z};
			SetPosition(pos);
		}

		inline void SetPosition(const glm::vec3 pos) {
			m_pos = pos;
			RebuildMatrix(UpdateType::TRANSLATION);
		};

		inline void SetScale(const glm::vec3 scale) {
			m_scale = scale;
			RebuildMatrix(UpdateType::SCALE);
		};

		inline void SetOrientation(const glm::vec3 rot) {
			m_orientation = rot;
			RebuildMatrix(UpdateType::ORIENTATION);
		};

		inline void SetAbsoluteMode(bool mode) {
			m_is_absolute = mode;
			RebuildMatrix(UpdateType::ALL);
		}

		const glm::mat4x4& GetMatrix() const { return m_transform; };

		// Returns inherited position([0]), scale([1]), rotation ([2]) including this components transforms.
		std::array<glm::vec3, 3> GetAbsoluteTransforms() const;

		glm::vec3 GetPosition() const;
		glm::vec3 GetScale() const { return m_scale; };
		glm::vec3 GetOrientation() const { return m_orientation; };


		enum UpdateType {
			TRANSLATION = 0,
			SCALE = 1,
			ORIENTATION = 2,
			ALL = 3
		};

	private:

		// If true, transform will not take parent transforms into account when building matrix.
		bool m_is_absolute = false;

		void RebuildMatrix(UpdateType type);

		glm::mat4 m_transform = glm::mat4(1);
		glm::vec3 m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 m_orientation = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 m_pos = glm::vec3(0.0f, 0.0f, 0.0f);


	};

}