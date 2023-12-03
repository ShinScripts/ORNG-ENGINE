#include "Component.h"


namespace ORNG {
	class ParticleEmitterComponent : public Component {
		friend class ParticleSystem;
		friend class EditorLayer;
		friend class SceneRenderer;
	public:
		ParticleEmitterComponent(SceneEntity* p_entity) : Component(p_entity) { };

		// Maximum of 100,000 particles per emitter
		void SetNbParticles(unsigned num) {
			if (num > 100'000)
				throw std::exception("ParticleEmitterComponent::SetNbParticles failed, number provided larger than limit (100,000)");

			int dif = num - m_num_particles;
			m_num_particles = num;

			DispatchUpdateEvent(NB_PARTICLES_CHANGED, dif);
		}

		unsigned GetNbParticles() {
			return m_num_particles;
		}

		void SetSpawnExtents(glm::vec3 extents) {
			m_spawn_extents = extents;
			DispatchUpdateEvent();
		}

		glm::vec3 GetSpawnExtents() {
			return m_spawn_extents;
		}

		void SetVelocityScale(glm::vec2 min_max) {
			m_velocity_min_max_scalar = min_max;
			DispatchUpdateEvent();
		}

		glm::vec2 GetVelocityScale() {
			return m_velocity_min_max_scalar;
		}

		void SetParticleLifespan(float lifespan_ms) {
			m_particle_lifespan_ms = lifespan_ms;
			DispatchUpdateEvent(LIFESPAN_CHANGED, (int)0);
		}

		float GetParticleLifespan() {
			return m_particle_lifespan_ms;
		}

		void SetParticleSpawnDelay(float delay_ms) {
			m_particle_spawn_delay_ms = delay_ms;
			DispatchUpdateEvent(SPAWN_DELAY_CHANGED, (int)0);
		}

		float GetSpawnDelay() {
			return m_particle_spawn_delay_ms;
		}

		void SetSpread(float spread) {
			m_spread = spread;
			DispatchUpdateEvent();
		}

		float GetSpread() {
			return m_spread;
		}

		void SetActive(bool active) {
			m_active = active;
			DispatchUpdateEvent();
		}

		bool IsActive() {
			return m_active;
		}

		MeshAsset* p_particle_mesh = nullptr;
		std::vector<const Material*> materials;

	private:

		enum EmitterSubEvent {
			DEFAULT,
			NB_PARTICLES_CHANGED,
			LIFESPAN_CHANGED,
			SPAWN_DELAY_CHANGED
		};

		void DispatchUpdateEvent(EmitterSubEvent se = DEFAULT, std::any data_payload = 0.f) {
			Events::ECS_Event<ParticleEmitterComponent> evt;
			evt.affected_components[0] = this;
			evt.event_type = Events::ECS_EventType::COMP_UPDATED;
			evt.sub_event_type = se;
			evt.data_payload = data_payload;

			Events::EventManager::DispatchEvent(evt);
		}
		// User-configurable parameters


		glm::vec3 m_spawn_extents = glm::vec3(50, 0, 50);
		glm::vec2 m_velocity_min_max_scalar = { 0.1, 50.0 };
		float m_particle_lifespan_ms = 1000.f;
		float m_particle_spawn_delay_ms = 1000.f / 64'000;
		float m_spread = 1.0; // 1 = 360 degree spread, 0 = no spread
		bool m_active = true;


		// State

		// Index into the SSBO's in ParticleSystem
		unsigned m_particle_start_index = 0;
		unsigned m_index = 0;
		unsigned m_num_particles = 64'000;
	};
}