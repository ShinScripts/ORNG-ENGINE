#pragma once
#include "util/util.h"
#include "assets/Asset.h"
#include "ScriptShared.h"

namespace ORNG {
	class SceneEntity;
	class Input;
	class FrameTiming;
	class Scene;

	namespace Events {
		class EventManager;
	}

	typedef void(__cdecl* ScriptFuncPtr)(SceneEntity*);
	typedef void(__cdecl* PhysicsEventCallback)(SceneEntity*, SceneEntity*);
	// Function ptr setters that enable the script to call some engine functions without dealing with certain unwanted libs/includes
	typedef void(__cdecl* InputSetter)(Input*);
	typedef void(__cdecl* EventInstanceSetter)(Events::EventManager*);
	typedef void(__cdecl* FrameTimingSetter)(FrameTiming*);
	typedef void(__cdecl* CreateEntitySetter)(std::function<SceneEntity& (const std::string&)>);
	typedef void(__cdecl* DeleteEntitySetter)(std::function<void(SceneEntity* p_entity)>);
	typedef void(__cdecl* GetEntitySetter)(std::function<SceneEntity& (uint64_t entity_uuid)>);
	typedef void(__cdecl* DuplicateEntitySetter)(std::function<SceneEntity& (SceneEntity& p_entity)>);
	typedef void(__cdecl* InstantiatePrefabSetter)(std::function<SceneEntity& (const std::string&)>);
	typedef void(__cdecl* RaycastSetter)(std::function<ORNG::RaycastResults(glm::vec3 origin, glm::vec3 unit_dir, float max_distance)>);
	typedef ScriptBase* (__cdecl* InstanceCreator)();
	typedef void(__cdecl* InstanceDestroyer)(ScriptBase*);




	struct ScriptSymbols {
		// Even if script fails to load, path must be preserved
		ScriptSymbols(const std::string& path) : script_path(path) {};

		bool loaded = false;
		std::string script_path;
		InstanceCreator CreateInstance = [] {return new ScriptBase(); };
		InstanceDestroyer DestroyInstance = [](ScriptBase* p_base) { delete p_base; };

		// These set the appropiate callback functions in the scripts DLL so they can modify the scene, the scene class will call these methods during Update
		// Would like to expose the entire scene class but due to the amount of dependencies I would need to include doing this for now
		CreateEntitySetter SceneEntityCreationSetter = [](std::function<SceneEntity& (const std::string&)>) {};
		DeleteEntitySetter SceneEntityDeletionSetter = [](std::function<void(SceneEntity* p_entity)>) {};
		DuplicateEntitySetter SceneEntityDuplicationSetter = [](std::function<SceneEntity& (SceneEntity& p_entity)>) {};
		InstantiatePrefabSetter ScenePrefabInstantSetter = [](std::function<SceneEntity& (const std::string&)>) {};
		RaycastSetter SceneRaycastSetter = [](std::function<ORNG::RaycastResults(glm::vec3 origin, glm::vec3 unit_dir, float max_distance)>) {};
		GetEntitySetter SceneGetEntitySetter = [](std::function<SceneEntity& (uint64_t)>) {};
	};


	class ScriptingEngine {
	public:
		static ScriptSymbols GetSymbolsFromScriptCpp(const std::string& filepath, bool precompiled);
		static ScriptSymbols LoadScriptDll(const std::string& dll_path, const std::string& relative_path);
		// Produces a path that a scripts dll will be stored in
		static std::string GetDllPathFromScriptCpp(const std::string& script_filepath);
		static bool UnloadScriptDLL(const std::string& filepath);
	private:
		// Filepaths always absolute and use "\" as directory seperators
		inline static std::map<std::string, HMODULE> sm_loaded_script_dll_handles;
	};

	struct ScriptAsset : public Asset {
		ScriptAsset(ScriptSymbols& t_symbols) : Asset(t_symbols.script_path), symbols(t_symbols) { };
		ScriptAsset(const std::string& filepath) : Asset(filepath), symbols(ScriptingEngine::GetSymbolsFromScriptCpp(filepath, true)) {  };

		ScriptSymbols symbols;
	};
}