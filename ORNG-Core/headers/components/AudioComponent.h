#ifndef AUDIOCOMP_H
#define AUDIOCOMP_H
#include "Component.h"

namespace FMOD {
	class Channel;
}
struct FMOD_VECTOR;

namespace ORNG {
	constexpr uint64_t INVALID_SOUND_UUID = 0;

	class SoundAsset;

	namespace Events {
		enum class ECS_EventType;
	}

	class AudioComponent : public Component {
		friend class AudioSystem;
		friend class EditorLayer;
		friend class SceneSerializer;
	public:
		AudioComponent() = delete;
		explicit AudioComponent(SceneEntity* p_entity) : Component(p_entity) { };


		void SetSoundAssetUUID(uint64_t uuid) {
			m_sound_asset_uuid = uuid;
		}

		void Play(uint64_t uuid = INVALID_SOUND_UUID);

		struct AudioRange {
			AudioRange(float t_min, float t_max) : min(t_min), max(t_max) {};
			float min;
			float max;
		};

		void SetPaused(bool b);
		void SetPitch(float p);
		void SetMinMaxRange(float min, float max);
		void SetVolume(float v);
		void SetPlaybackPosition(float time_in_ms);
		void SetLooped(bool looped);

		float GetVolume();
		float GetPitch();
		AudioRange GetMinMaxRange();
		bool IsPlaying();
		bool IsPaused();
		// Returns playback position in ms
		unsigned int GetPlaybackPosition();
		uint64_t GetAudioAssetUUID() { return m_sound_asset_uuid; }

		enum class AudioEventType {
			PLAY,
		};

	private:
		void DispatchAudioEvent(std::any data, Events::ECS_EventType event_type, uint32_t sub_event_type);

		uint64_t m_sound_asset_uuid = INVALID_SOUND_UUID;

		// Have to store a copy of this state here because it's not retrievable in the channel if the channel isn't actively playing
		float m_volume = 1.f;
		float m_pitch = 1.f;
		bool is_looped = false;
		AudioRange m_range{ 0.1f, 10000.f };


		// Memory managed by AudioSystem
		FMOD_VECTOR* mp_fmod_pos = nullptr;
		FMOD_VECTOR* mp_fmod_vel = nullptr;

		FMOD::Channel* mp_channel = nullptr;
	};
}

#endif