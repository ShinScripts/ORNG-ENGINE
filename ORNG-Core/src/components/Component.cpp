#include "pch/pch.h"
#include "components/Component.h"
#include "scene/SceneEntity.h"
#include "events/EventManager.h"

namespace ORNG {
	Component::Component(SceneEntity* p_entity) : mp_entity(p_entity) {};
	uint64_t Component::GetEntityUUID() const { return mp_entity->GetUUID(); }
	entt::entity Component::GetEnttHandle() const { return mp_entity->GetEnttHandle(); }
	uint64_t Component::GetSceneUUID() const { return mp_entity->GetScene()->uuid(); }
	std::string Component::GetEntityName() const { return mp_entity->name; }
}