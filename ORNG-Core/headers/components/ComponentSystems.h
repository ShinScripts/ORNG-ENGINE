#pragma once
#include "components/ComponentAPI.h"
#include "events/EventManager.h"
#include "rendering/Textures.h"
#include "physx/PxPhysicsAPI.h"
#include "physx/vehicle2/PxVehicleAPI.h"
#include "scripting/ScriptShared.h"
#include "rendering/VAO.h"
#include "components/ParticleBufferComponent.h"

namespace physx {
	class PxScene;
	class PxBroadPhase;
	class PxAABBManager;
	class PxCpuDispatcher;
	class PxMaterial;
	class PxTriangleMesh;
	class PxControllerManager;
}

namespace FMOD {
	class ChannelGroup;
}

namespace ORNG {
	class Scene;
	class MeshInstanceGroup;
	class ShaderVariants;

	class ComponentSystem {
	public:
		explicit ComponentSystem(entt::registry* p_registry, uint64_t scene_uuid) : mp_registry(p_registry), m_scene_uuid(scene_uuid) {};
		// Dispatches event attached to single component, used for connections with entt::registry::on_construct etc
		template<std::derived_from<Component> T>
		static void DispatchComponentEvent(entt::registry& registry, entt::entity entity, Events::ECS_EventType type) {
			Events::ECS_Event<T> e_event{ type, &registry.get<T>(entity) };
			Events::EventManager::DispatchEvent(e_event);
		}

		inline uint64_t GetSceneUUID() const { return m_scene_uuid; }
	protected:
		entt::registry* mp_registry = nullptr;
		uint64_t m_scene_uuid = 0;
	};



	class AudioSystem : public ComponentSystem {
	public:
		AudioSystem(entt::registry* p_registry, uint64_t scene_uuid, entt::entity* p_active_cam_id) : ComponentSystem(p_registry, scene_uuid), mp_active_cam_id(p_active_cam_id) {};
		void OnLoad();
		void OnUnload();
		void OnUpdate();
	private:

		void OnAudioDeleteEvent(const Events::ECS_Event<AudioComponent>& e_event);
		void OnAudioUpdateEvent(const Events::ECS_Event<AudioComponent>& e_event);
		void OnAudioAddEvent(const Events::ECS_Event<AudioComponent>& e_event);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& e_event);

		Events::ECS_EventListener<AudioComponent> m_audio_listener;
		Events::ECS_EventListener<TransformComponent> m_transform_listener;

		FMOD::ChannelGroup* mp_channel_group = nullptr;

		// Points to memory in scene's "CameraSystem" to find active camera
		entt::entity* mp_active_cam_id;
	};



	class TransformHierarchySystem : public ComponentSystem {
	public:
		TransformHierarchySystem(entt::registry* p_registry, uint64_t scene_uuid) : ComponentSystem(p_registry, scene_uuid) {};
		void OnLoad();

		void OnUnload() {
			Events::EventManager::DeregisterListener((entt::entity)m_transform_event_listener.GetRegisterID());
		}
	private:
		void UpdateChildTransforms(const Events::ECS_Event<TransformComponent>&);
		Events::ECS_EventListener<TransformComponent> m_transform_event_listener;
	};


	class CameraSystem : public ComponentSystem {
		friend class Scene;
	public:
		CameraSystem(entt::registry* p_registry, uint64_t scene_uuid) : ComponentSystem(p_registry, scene_uuid) {
			m_event_listener.OnEvent = [this](const Events::ECS_Event<CameraComponent>& t_event) {
				if (t_event.event_type == Events::ECS_EventType::COMP_UPDATED && t_event.affected_components[0]->is_active) {
					SetActiveCamera(t_event.affected_components[0]->GetEnttHandle());
				}
				};

			m_event_listener.scene_id = GetSceneUUID();
			Events::EventManager::RegisterListener(m_event_listener);
		};
		virtual ~CameraSystem() = default;

		void OnUnload() {};
		void OnLoad() {
		}

		void OnUpdate() {
			auto* p_active_cam = GetActiveCamera();
			if (p_active_cam)
				p_active_cam->Update();
		}

		void SetActiveCamera(entt::entity entity_handle) {
			m_active_cam_entity_handle = entity_handle;
			auto view = mp_registry->view<CameraComponent>();

			// Make all other cameras inactive
			for (auto [entity, camera] : view.each()) {
				if (entity != entity_handle)
					camera.is_active = false;
			}
		}

		// Returns ptr to active camera or nullptr if no camera is active.
		CameraComponent* GetActiveCamera() {
			if (mp_registry->all_of<CameraComponent>((entt::entity)m_active_cam_entity_handle))
				return &mp_registry->get<CameraComponent>((entt::entity)m_active_cam_entity_handle);
			else
				return nullptr;
		}

	private:
		entt::entity m_active_cam_entity_handle;
		Events::ECS_EventListener<CameraComponent> m_event_listener;
	};






	class PhysicsSystem : public ComponentSystem {
		friend class EditorLayer;
	public:
		PhysicsSystem(entt::registry* p_registry, uint64_t scene_uuid, Scene* p_scene);
		virtual ~PhysicsSystem() = default;

		void OnUpdate(float ts);
		void OnUnload();
		void OnLoad();
		physx::PxTriangleMesh* GetOrCreateTriangleMesh(const MeshAsset* p_mesh_data);

		RaycastResults Raycast(glm::vec3 origin, glm::vec3 unit_dir, float max_distance);
		OverlapQueryResults OverlapQuery(PxGeometry& geom, glm::vec3 pos, unsigned max_hits);

		enum class ActorType : uint8_t {
			RIGID_BODY,
			CHARACTER_CONTROLLER,
			VEHICLE
		};


		bool InitVehicle(VehicleComponent* p_comp);


	private:
		static void UpdateTransformCompFromGlobalPose(const PxTransform& pose, TransformComponent& transform, PhysicsSystem::ActorType type);

		void InitComponent(PhysicsComponent* p_comp);
		void InitComponent(CharacterControllerComponent* p_comp);
		void InitComponent(VehicleComponent* p_comp);
		void InitComponent(JointComponent* p_comp);

		void HandleComponentUpdate(const Events::ECS_Event<JointComponent>& t_event);
		void UpdateComponentState(PhysicsComponent* p_comp);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event);

		void RemoveComponent(PhysicsComponent* p_comp);
		void RemoveComponent(CharacterControllerComponent* p_comp);
		void RemoveComponent(JointComponent* p_comp);
		void RemoveComponent(VehicleComponent* p_comp);

		void ConnectJoint(const JointComponent::ConnectionData& connection);
		void BreakJoint(JointComponent::Joint* p_joint);


		void InitVehicleSimulationContext();

		physx::vehicle2::PxVehiclePhysXSimulationContext m_vehicle_context;
		PxConvexMesh* mp_sweep_mesh = nullptr;

		void InitListeners();
		void DeinitListeners();


		Scene* mp_scene = nullptr;

		Events::ECS_EventListener<PhysicsComponent> m_phys_listener;
		Events::ECS_EventListener<JointComponent> m_joint_listener;
		Events::ECS_EventListener<CharacterControllerComponent> m_character_controller_listener;
		Events::ECS_EventListener<TransformComponent> m_transform_listener;
		Events::ECS_EventListener<VehicleComponent> m_vehicle_listener;


		physx::PxBroadPhase* mp_broadphase = nullptr;
		physx::PxAABBManager* mp_aabb_manager = nullptr;
		physx::PxScene* mp_phys_scene = nullptr;
		physx::PxControllerManager* mp_controller_manager = nullptr;

		std::unordered_map<const MeshAsset*, physx::PxTriangleMesh*> m_triangle_meshes;

		// Joints that have been broken during the simulation and logged with onConstraintBreak are stored here to disconnect them from entities after simulate() has finished
		std::vector<JointComponent::Joint*> m_joints_to_break;

		// Need to store joint currently being processed by BreakJoint() as it will trigger onConstraintBreak which will call BreakJoint() again if this isn't checked
		JointComponent::Joint* mp_currently_breaking_joint = nullptr;

		// Queue of entities that need OnCollision script events (if they have one) to fire, has to be done outside of simulation due to restrictions with rigidbody modification during simulation, processed each frame in OnUpdate
		std::vector<std::pair<entt::entity, entt::entity>> m_entity_collision_queue;

		enum TriggerEvent {
			ENTERED,
			EXITED
		};

		std::vector<std::tuple<TriggerEvent, entt::entity, entt::entity>> m_trigger_event_queue;

		// Transform that is currently being updated by the physics system, used to prevent needless physics component updates
		TransformComponent* mp_currently_updating_transform = nullptr;

		static constexpr float m_step_size = (1.f / 60.f);
		float m_accumulator = 0.f;

		class PhysCollisionCallback : public physx::PxSimulationEventCallback {
		public:
			PhysCollisionCallback(PhysicsSystem* p_system) : mp_system(p_system) {};

			virtual void onConstraintBreak([[maybe_unused]] PxConstraintInfo* constraints, [[maybe_unused]] PxU32 count) override;

			virtual void onWake([[maybe_unused]] PxActor** actors, [[maybe_unused]] PxU32 count) override {};

			virtual void onSleep[[maybe_unused]] (PxActor** actors, [[maybe_unused]] PxU32 count) override {};

			virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;

			virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override;

			virtual void onAdvance([[maybe_unused]] const PxRigidBody* const* bodyBuffer, [[maybe_unused]] const PxTransform* poseBuffer, [[maybe_unused]] const PxU32 count) override {};


		private:
			// Used for its PxActor entity lookup map
			PhysicsSystem* mp_system = nullptr;
		};

		PhysCollisionCallback m_collision_callback{ this };
	};


	class SpotlightSystem {
		friend class SceneRenderer;
	public:
		SpotlightSystem() = default;
		virtual ~SpotlightSystem() = default;

		void OnLoad();
		void OnUpdate(entt::registry* p_registry);
		void OnUnload();
	private:

		void WriteLightToVector(std::vector<float>& output_vec, SpotLightComponent& light, int& index);
		Texture2DArray m_spotlight_depth_tex{
		"Spotlight depth"
		}; // Used for shadow maps
		SSBO<float> m_spotlight_ssbo{true, 0};
	};



	class PointlightSystem {
		friend class SceneRenderer;
	public:
		PointlightSystem() = default;
		~PointlightSystem() = default;
		void OnLoad();
		void OnUpdate(entt::registry* p_registry);
		void OnUnload();
		void WriteLightToVector(std::vector<float>& output_vec, PointLightComponent& light, int& index);

		// Checks if the depth map array needs to grow/shrink
		void OnDepthMapUpdate();

		static constexpr unsigned POINTLIGHT_SHADOW_MAP_RES = 2048;

	private:
		TextureCubemapArray m_pointlight_depth_tex{ "Pointlight depth" }; // Used for shadow maps
		SSBO<float> m_pointlight_ssbo{ true, 0 };

	};


	class MeshInstancingSystem : public ComponentSystem {
	public:

		MeshInstancingSystem(entt::registry* p_registry, uint64_t scene_uuid);
		virtual ~MeshInstancingSystem() = default;
		void SortMeshIntoInstanceGroup(MeshComponent* comp);
		void OnLoad();
		void OnUnload();
		void OnUpdate();
		void OnMeshEvent(const Events::ECS_Event<MeshComponent>& t_event);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event);

		const auto& GetInstanceGroups() const { return m_instance_groups; }
		const auto& GetBillboardInstanceGroups() const { return m_billboard_instance_groups; }
	private:
		void OnMeshAssetDeletion(MeshAsset* p_asset);
		void OnMaterialDeletion(Material* p_material);

		void SortBillboardIntoInstanceGroup(BillboardComponent* p_comp);
		void OnBillboardAdd(BillboardComponent* p_comp);
		void OnBillboardRemove(BillboardComponent* p_comp);

		// Listener for asset deletion
		Events::EventListener<Events::AssetEvent> m_asset_listener;

		Events::ECS_EventListener<TransformComponent> m_transform_listener;
		Events::ECS_EventListener<MeshComponent> m_mesh_listener;
		std::vector<MeshInstanceGroup*> m_instance_groups;

		Events::ECS_EventListener<BillboardComponent> m_billboard_listener;
		std::vector<MeshInstanceGroup*> m_billboard_instance_groups;

		unsigned m_default_group_end_index = 0;
	};

	class ParticleBufferComponent;

	class Shader;
	class ShaderVariants;

	class ParticleSystem : public ComponentSystem {
		friend class EditorLayer;
		friend class SceneRenderer;
	public:
		ParticleSystem(entt::registry* p_registry, uint64_t scene_uuid);
		void OnLoad();
		void OnUnload();
		void OnUpdate();
	private:
		void InitEmitter(ParticleEmitterComponent* p_comp);
		void OnEmitterUpdate(const Events::ECS_Event<ParticleEmitterComponent>& e_event);
		void OnEmitterUpdate(ParticleEmitterComponent* p_comp);
		void OnEmitterDestroy(ParticleEmitterComponent* p_comp, unsigned dif = 0);

		void UpdateAppendBuffer();

		void InitBuffer(ParticleBufferComponent* p_comp);
		void OnBufferDestroy(ParticleBufferComponent* p_comp);
		void OnBufferUpdate(ParticleBufferComponent* p_comp);

		void OnEmitterVisualTypeChange(ParticleEmitterComponent* p_comp);

		void UpdateEmitterBufferAtIndex(unsigned index);

		Events::ECS_EventListener<ParticleEmitterComponent> m_particle_listener;
		Events::ECS_EventListener<ParticleBufferComponent> m_particle_buffer_listener;

		Events::ECS_EventListener<TransformComponent> m_transform_listener;

		// Stored in order based on their m_particle_start_index
		std::vector<entt::entity> m_emitter_entities;

		// Total particles belonging to emitters, does not include those belonging to ParticleBufferComponents which are stored separately
		unsigned total_emitter_particles = 0;

		unsigned* p_num_appended = nullptr;
		std::byte* p_emitter_gpu_buffer = nullptr;

		SSBO<float> m_particle_ssbo{ false, 0 };
		SSBO<float> m_emitter_ssbo{ false, GL_DYNAMIC_STORAGE_BIT };
		SSBO<float> m_particle_append_ssbo{ false, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT };

		SSBO<float> m_particle_copy_buffer{ false, 0 };


		inline static Shader* mp_particle_cs;
		inline static ShaderVariants* mp_particle_initializer_cs;
		inline static Shader* mp_append_buffer_transfer_cs;
	};


	struct BillboardInstanceGroup {
		unsigned num_instances = 0;
		Material* p_material = nullptr;

		// Transforms stored as two vec3's (pos, scale)
		SSBO<float> transform_ssbo;
	};
}