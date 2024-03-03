#ifndef EVENTS_H
#define EVENTS_H
#include "components/Component.h"


namespace ORNG {
	class SceneEntity;

	enum MouseButton {
		LEFT_BUTTON = 0,
		RIGHT_BUTTON = 1,
		SCROLL = 2,
		NONE = 3,
	};

	enum MouseAction {
		UP = 0,
		DOWN = 1,
		MOVE = 2,
		TOGGLE_VISIBILITY,
	};

	enum InputType : uint8_t {
		RELEASE = 0, // GLFW_RELEASE
		PRESS = 1, // GLFW_PRESS
		HELD = 2 // GLFW_REPEAT
	};
}

namespace ORNG::Events {
	struct Event {
		enum EventType {
			INVALID_TYPE = 0,
			ENGINE_RENDER = 1,
			ENGINE_UPDATE = 2,
			WINDOW_RESIZE = 3,
		};


		EventType event_type = EventType::INVALID_TYPE;
	};

	struct EngineCoreEvent : public Event {
	};

	struct WindowEvent : public Event {
		glm::ivec2 old_window_size;
		glm::ivec2 new_window_size;
	};


	enum MouseEventType {
		RECEIVE, // Input received
		SET // State/window should be updated, e.g change cursor pos, listened for and handled in Window class
	};

	struct MouseEvent : public Event {
		MouseEvent(MouseEventType _event_type, MouseAction _action, MouseButton _button, glm::ivec2 _new_cursor_pos, glm::ivec2 _old_cursor_pos, std::any _data_payload = 0) :
			event_type(_event_type), mouse_action(_action), mouse_button(_button), mouse_pos_new(_new_cursor_pos), mouse_pos_old(_old_cursor_pos), data_payload(_data_payload) {};

		MouseEventType event_type;
		MouseAction mouse_action;
		MouseButton mouse_button;
		glm::ivec2 mouse_pos_new;
		glm::ivec2 mouse_pos_old;
		std::any data_payload;
	};


	struct KeyEvent : public Event {
		KeyEvent(unsigned _key, uint8_t _event_type) : key(_key), event_type(_event_type) {};

		unsigned int key;
		uint8_t event_type; // will be InputType enum, can't declare as such due to circular include
	};

	enum class AssetEventType {
		MATERIAL_DELETED,
		MESH_DELETED,
		MESH_LOADED,
		MATERIAL_LOADED,
		TEXTURE_DELETED,
		TEXTURE_LOADED,
		SCRIPT_DELETED,
	};

	struct AssetEvent : public Event {
		AssetEventType event_type;
		uint8_t* data_payload = nullptr; // Data payload will be a ptr to the asset being modified if it's an asset event
	};

	enum class ECS_EventType {
		COMP_ADDED,
		COMP_UPDATED,
		COMP_DELETED,
	};

	template <std::derived_from<Component> T>
	struct ECS_Event : public Event {
		ECS_Event(ECS_EventType t_event_type, T* p_comp, uint32_t t_sub_event_type = UINT32_MAX) : event_type(t_event_type), sub_event_type(t_sub_event_type) { affected_components[0] = p_comp; };
		ECS_EventType event_type;
		uint32_t sub_event_type; // E.g a code for "Scaling transform" for a transform component update

		std::array<T*, 2> affected_components = { nullptr, nullptr };
		std::any data_payload;
	};

	template <std::derived_from<Event> T>
	class EventListener {
	public:
		friend class EventManager;
		EventListener() = default;
		EventListener(const EventListener&) = default;
		~EventListener() { if (OnDestroy) OnDestroy(); }
		std::function<void(const T&)> OnEvent = nullptr;

		// Handle used for deregistration, set by eventmanager on listener registration
		entt::entity GetRegisterID() { return m_entt_handle; }
	private:
		// Given upon listener being registered with EventManager
		entt::entity m_entt_handle = entt::entity(0);
		// Function given by event manager when listener is registered
		std::function<void()> OnDestroy = nullptr;
	};


	template<std::derived_from<Component> T>
	class ECS_EventListener : public EventListener<ECS_Event<T>> {
	public:
		uint64_t scene_id = 0;
	};


#ifdef ORNG_EDITOR_LAYER
	enum class EditorEventType {
		POST_INITIALIZATION,
		POST_SCENE_LOAD,
		SCENE_START_SIMULATION,
		SCENE_END_SIMULATION
	};

	struct EditorEvent : public Event {
		EditorEvent(EditorEventType type) : event_type(type) {};
		EditorEventType event_type;
	};

#endif
}

#endif