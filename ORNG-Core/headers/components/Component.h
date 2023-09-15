#ifndef COMPONENT_H
#define COMPONENT_H
#include "entt/EnttSingleInclude.h"
namespace ORNG {

	class SceneEntity;

	class Component {
	public:
		friend class Scene;
		Component(SceneEntity* p_entity);


		uint64_t GetEntityUUID() const;
		entt::entity GetEnttHandle() const;
		uint64_t GetSceneUUID() const;
		SceneEntity* GetEntity() { return mp_entity; }
		std::string GetEntityName() const;

	private:
		SceneEntity* mp_entity = nullptr;
	};

	struct RelationshipComponent : public Component {
		RelationshipComponent(SceneEntity* p_entity) : Component(p_entity) {};
		size_t num_children = 0;
		entt::entity first{ entt::null };
		entt::entity prev{ entt::null };
		entt::entity next{ entt::null };
		entt::entity parent{ entt::null };
	};

	template<typename T>
	struct DataComponent : public Component {
		std::unordered_map<std::string, T> data;
	};
}

#endif