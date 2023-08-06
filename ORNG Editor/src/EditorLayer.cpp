#include "pch/pch.h"


#include "../extern/Icons.h"
#include "../extern/imgui/backends/imgui_impl_glfw.h"
#include "../extern/imgui/backends/imgui_impl_opengl3.h"
#include "../extern/imgui/misc/cpp/imgui_stdlib.h"
#include "EditorLayer.h"
#include "scene/SceneSerializer.h"
#include "util/Timers.h"
#include "../extern/imguizmo/ImGuizmo.h"
#include "core/CodedAssets.h"


namespace ORNG {

	void EditorLayer::Init() {


		InitImGui();
		m_active_scene = std::make_unique<Scene>();


		m_grid_mesh = std::make_unique<GridMesh>();
		m_grid_mesh->Init();
		mp_grid_shader = &Renderer::GetShaderLibrary().CreateShader("grid");
		mp_grid_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/GridVS.glsl");
		mp_grid_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/GridFS.glsl");
		mp_grid_shader->Init();

		mp_quad_shader = &Renderer::GetShaderLibrary().CreateShader("2d_quad");
		mp_quad_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::QuadVS);
		mp_quad_shader->AddStageFromString(GL_FRAGMENT_SHADER, CodedAssets::QuadFS);
		mp_quad_shader->Init();

		mp_picking_shader = &Renderer::GetShaderLibrary().CreateShader("picking");
		mp_picking_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::TransformVS);
		mp_picking_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/PickingFS.glsl");
		mp_picking_shader->Init();
		mp_picking_shader->AddUniform("comp_id");
		mp_picking_shader->AddUniform("transform");

		mp_highlight_shader = &Renderer::GetShaderLibrary().CreateShader("highlight");
		mp_highlight_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::TransformVS);
		mp_highlight_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/HighlightFS.glsl");
		mp_highlight_shader->Init();
		mp_highlight_shader->AddUniform("transform");

		// Setting up the scene display texture 
		Texture2DSpec color_render_texture_spec;
		color_render_texture_spec.format = GL_RGBA;
		color_render_texture_spec.internal_format = GL_RGBA16F;
		color_render_texture_spec.storage_type = GL_FLOAT;
		color_render_texture_spec.mag_filter = GL_NEAREST;
		color_render_texture_spec.min_filter = GL_NEAREST;
		color_render_texture_spec.width = Window::GetWidth();
		color_render_texture_spec.height = Window::GetHeight();
		color_render_texture_spec.wrap_params = GL_CLAMP_TO_EDGE;

		mp_scene_display_texture = std::make_unique<Texture2D>("Editor scene display");
		mp_scene_display_texture->SetSpec(color_render_texture_spec);

		// Adding a resize event listener so the scene display texture scales with the window
		m_window_event_listener.OnEvent = [this](const Events::WindowEvent& t_event) {
			if (t_event.event_type == Events::Event::EventType::WINDOW_RESIZE) {
				auto spec = mp_scene_display_texture->GetSpec();
				spec.width = t_event.new_window_size.x;
				spec.height = t_event.new_window_size.y;
				mp_scene_display_texture->SetSpec(spec);
				mp_alt_scene_display_texture->SetSpec(spec);
			}
		};

		Events::EventManager::RegisterListener(m_window_event_listener);

		Texture2DSpec picking_spec;
		picking_spec.format = GL_RG_INTEGER;
		picking_spec.internal_format = GL_RG32UI;
		picking_spec.storage_type = GL_UNSIGNED_INT;
		picking_spec.width = Window::GetWidth();
		picking_spec.height = Window::GetHeight();

		// Entity ID's are split into halves for storage in textures then recombined later as there is no format for 64 bit uints
		mp_picking_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("picking", true);
		mp_picking_fb->AddRenderbuffer(Window::GetWidth(), Window::GetHeight());
		mp_picking_fb->Add2DTexture("component_ids_split", GL_COLOR_ATTACHMENT0, picking_spec);

		SceneRenderer::SetActiveScene(&*m_active_scene);

		mp_editor_pass_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("editor_passes", true);
		mp_editor_pass_fb->AddShared2DTexture("shared_depth", Renderer::GetFramebufferLibrary().GetFramebuffer("gbuffer").GetTexture<Texture2D>("shared_depth"), GL_DEPTH_ATTACHMENT);
		mp_editor_pass_fb->AddShared2DTexture("Editor scene display", *mp_scene_display_texture, GL_COLOR_ATTACHMENT0);

		m_active_scene->LoadScene("scene.yml");
		mp_alt_scene_display_texture = std::make_unique<Texture2D>("Alt");
		mp_alt_scene_display_texture->SetSpec(color_render_texture_spec);
		// Setup preview scene used for viewing materials on meshes
		mp_preview_scene = std::make_unique<Scene>();
		mp_preview_scene->LoadScene("");
		auto* p_sphere = mp_preview_scene->CreateMeshAsset("./res/meshes/sphere.fbx");
		p_sphere->LoadMeshData();
		mp_preview_scene->LoadMeshAssetIntoGPU(p_sphere);
		auto& cube_entity = mp_preview_scene->CreateEntity("Editor preview cube");
		auto& cam_entity = mp_preview_scene->CreateEntity("Editor preview cam");
		auto* p_cam = cam_entity.AddComponent<CameraComponent>();
		auto* p_mesh = cube_entity.AddComponent<MeshComponent>(p_sphere);
		cube_entity.GetComponent<TransformComponent>()->SetScale(10, 10, 10);
		p_cam->GetEntity()->GetComponent<TransformComponent>()->SetPosition({ 0, 0, 10 });
		p_cam->target = { 0, 0, 1 };
		//p_cam->MakeActive();
		auto* p_mat = mp_preview_scene->CreateMaterial();
		p_mat->base_color = { 0, 0, 0 };
		p_mat->shader_id = 2;
		p_mesh->SetMaterialID(0, p_mat);

		auto& cube_entity_2 = m_active_scene->CreateEntity("Editor preview cube");
		cube_entity_2.GetComponent<TransformComponent>()->SetScale(10, 10, 10);
		auto* p_mesh_2 = cube_entity_2.AddComponent<MeshComponent>(p_sphere);
		p_mesh_2->SetMaterialID(0, p_mat);
		mp_editor_camera = std::make_unique<SceneEntity>(&*m_active_scene, m_active_scene->m_registry.create());
		mp_editor_camera->AddComponent<TransformComponent>()->SetPosition(0, 20, 0);
		mp_editor_camera->AddComponent<EditorCamera>();
		mp_editor_camera->AddComponent<CharacterControllerComponent>();
		//mp_editor_camera->GetComponent<EditorCamera>()->MakeActive();

		ORNG_CORE_INFO("Editor layer initialized");
	}





	void EditorLayer::InitImGui() {
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.Fonts->AddFontDefault();
		io.FontDefault = io.Fonts->AddFontFromFileTTF("./res/fonts/PlatNomor-WyVnn.ttf", 18.0f);
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		ImFontConfig config;
		config.MergeMode = true;
		static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		io.Fonts->AddFontFromFileTTF("./res/fonts/fa-regular-400.ttf", 18.0f, &config, icon_ranges);
		io.Fonts->AddFontFromFileTTF("./res/fonts/fa-solid-900.ttf", 18.0f, &config, icon_ranges);
		ImGui_ImplOpenGL3_CreateFontsTexture();
		ImGui::StyleColorsDark();


		ImGui::GetStyle().Colors[ImGuiCol_Button] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_Border] = dark_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_Tab] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TabHovered] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TabActive] = orange_color;
		ImGui::GetStyle().Colors[ImGuiCol_Header] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] = orange_color;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = dark_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_FrameBgActive] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_DockingEmptyBg] = lightest_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = lighter_grey_color;
		ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = lighter_grey_color;
	}


	void EditorLayer::Update() {
		ORNG_PROFILE_FUNC();
		if (Window::IsKeyDown('K'))
			mp_editor_camera->GetComponent<EditorCamera>()->MakeActive();

		float ts = FrameTiming::GetTimeStep();

		if (!ImGui::GetIO().WantTextInput && !(ImGui::GetIO().WantCaptureMouse && Window::GetScrollStatus().active)) {
			mp_editor_camera->GetComponent<EditorCamera>()->Update();
		}

		m_active_scene->Update(ts);
		mp_preview_scene->Update(ts);


		static float cooldown = 0;
		cooldown -= glm::min(cooldown, ts);
		if (cooldown > 10)
			return;
		if (Window::IsKeyDown('J'))
			CreateMaterialPreview(m_active_scene->m_materials[0]);

		// Keybind to focus on selected entities
		if (Window::IsKeyDown('F') && !m_selected_entity_ids.empty()) {
			glm::vec3 avg_pos = { 0, 0, 0 };
			int num_iters = 0;
			for (auto id : m_selected_entity_ids) {
				num_iters++;
				avg_pos += m_active_scene->GetEntity(id)->GetComponent<TransformComponent>()->m_pos;
			}
			mp_editor_camera->GetComponent<EditorCamera>()->target = glm::normalize(avg_pos / (float)num_iters - mp_editor_camera->GetComponent<TransformComponent>()->m_pos);
		}


		if (Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL) && Window::IsKeyDown('D')) {
			cooldown += 1000;

			std::vector<uint64_t> duplicate_ids;
			for (auto id : m_selected_entity_ids) {
				SceneEntity* p_entity = m_active_scene->GetEntity(id);
				if (!p_entity) continue;

				DuplicateEntity(p_entity);
				duplicate_ids.push_back(p_entity->GetUUID());
			}

			m_selected_entity_ids = duplicate_ids;
		}

	}

	Texture2D EditorLayer::CreateMaterialPreview(Material* p_material) {
		Scene temp_scene;
		ORNG_CORE_TRACE("DSDFDF");
		temp_scene.LoadScene("");
		return Texture2D("new");
	}

	void EditorLayer::RenderProfilingTimers() {
		static bool display_profiling_timers = ProfilingTimers::AreTimersEnabled();

		if (ImGui::Checkbox("Timers", &display_profiling_timers)) {
			ProfilingTimers::SetTimersEnabled(display_profiling_timers);
		}
		if (ProfilingTimers::AreTimersEnabled()) {
			for (auto& string : ProfilingTimers::GetTimerData()) {
				ImGui::Text(string.c_str());
			}
			ProfilingTimers::UpdateTimers(FrameTiming::GetTimeStep());
		}
	}

	void EditorLayer::RenderUI() {
		ORNG_PROFILE_FUNC();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(400, Window::GetHeight()));
		ImGui::Begin("##left window", (bool*)0, 0);
		ImGui::End();


		if (ImGui::Begin("Debug")) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text(std::format("Draw calls: {}", Renderer::Get().m_draw_call_amount).c_str());

			RenderProfilingTimers();

			Renderer::ResetDrawCallCounter();

		}
		ImGui::End();

		RenderSceneGraph();
		ShowAssetManager();
		RenderEditorWindow();

		// Drag popup
		if (mp_dragged_material) {
			ImGui::SetNextWindowPos(ImVec2(25 + ImGui::GetMousePos().x, 25 + ImGui::GetMousePos().y));
			ImGui::SetNextWindowSize(ImVec2(50, 50));
			ImGui::SetNextWindowBgAlpha(0.25f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::Begin("##dragging", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(ImTextureID(mp_dragged_material->base_color_texture ? mp_dragged_material->base_color_texture->GetTextureHandle() : CodedAssets::GetBaseTexture().GetTextureHandle()), ImVec2(50, 50));
			ImGui::End();
			ImGui::PopStyleVar();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	}

	SceneEntity* EditorLayer::DuplicateEntity(SceneEntity* p_original) {
		SceneEntity& new_entity = m_active_scene->CreateEntity(p_original->name + " - Duplicate");
		std::string str = SceneSerializer::SerializeEntityIntoString(*p_original);
		SceneSerializer::DeserializeEntityFromString(*m_active_scene, str, new_entity);

		return &new_entity;
	}



	void EditorLayer::RenderDisplayWindow() {
		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();
		if (Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) && !ImGui::GetIO().WantCaptureMouse) {
			DoPickingPass();
		}
		GL_StateManager::DefaultClearBits();

		SceneRenderer::SceneRenderingSettings settings;
		SceneRenderer::SetActiveScene(&*m_active_scene);
		settings.p_output_tex = &*mp_scene_display_texture;
		settings.p_cam_override = static_cast<CameraComponent*>(&*mp_editor_camera->GetComponent<EditorCamera>());
		SceneRenderer::SceneRenderingOutput output = SceneRenderer::RenderScene(settings);


		mp_editor_pass_fb->Bind();
		DoSelectedEntityHighlightPass();
		RenderGrid();

		Renderer::GetFramebufferLibrary().UnbindAllFramebuffers();
		mp_quad_shader->ActivateProgram();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_scene_display_texture->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR, true);

		// Render scene texture to editor display window (currently just fullscreen quad)
		glDisable(GL_DEPTH_TEST);
		Renderer::DrawQuad();
		glEnable(GL_DEPTH_TEST);

	}



	void EditorLayer::RenderCreationWidget(SceneEntity* p_entity, bool trigger) {

		const char* names[5] = { "Pointlight", "Spotlight", "Mesh", "Camera", "Physics" };

		int selected_component = -1;
		if (trigger)
			ImGui::OpenPopup("my_select_popup");

		if (ImGui::BeginPopup("my_select_popup"))
		{
			ImGui::SeparatorText("Create entity");
			for (int i = 0; i < IM_ARRAYSIZE(names); i++)
				if (ImGui::Selectable(names[i]))
					selected_component = i;
			ImGui::EndPopup();
		}

		if (selected_component == -1)
			return;

		auto* entity = p_entity ? p_entity : &m_active_scene->CreateEntity("New entity");
		SelectEntity(entity->GetUUID());

		switch (selected_component) {
		case 0:
			entity->AddComponent<PointLightComponent>();
			break;
		case 1:
			entity->AddComponent<SpotLightComponent>();
			break;
		case 2:
			entity->AddComponent<MeshComponent>(&CodedAssets::GetCubeAsset());
			break;
		case 3:
			entity->AddComponent<CameraComponent>();
			break;
		case 4:
			entity->AddComponent<PhysicsComponent>();
			break;
		}
	}




	void EditorLayer::RenderEditorWindow() {
		ImGui::SetNextWindowPos(ImVec2(Window::GetWidth() - 400, 0));
		ImGui::SetNextWindowSize(ImVec2(400, Window::GetHeight()));

		if (ImGui::Begin("Editor", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
			DisplayEntityEditor();

			if (mp_selected_material)
				RenderMaterialEditorSection();

			if (mp_selected_texture)
				RenderTextureEditorSection();

		}
		ImGui::End();
	}

	void EditorLayer::DoPickingPass() {

		mp_picking_fb->Bind();
		mp_picking_shader->ActivateProgram();

		GL_StateManager::ClearDepthBits();
		GL_StateManager::ClearBitsUnsignedInt();

		auto view = m_active_scene->m_registry.view<MeshComponent>();
		for (auto [entity, mesh] : view.each()) {
			//Split uint64 into two uint32's for texture storage
			uint64_t full_id = mesh.GetEntityUUID();
			uint32_t half_id_1 = (uint32_t)(full_id >> 32);
			uint32_t half_id_2 = (uint32_t)(full_id);
			glm::uvec2 id_vec{half_id_1, half_id_2};

			mp_picking_shader->SetUniform("comp_id", id_vec);
			mp_picking_shader->SetUniform("transform", mesh.GetEntity()->GetComponent<TransformComponent>()->GetMatrix());

			Renderer::DrawMeshInstanced(mesh.GetMeshData(), 1);
		}

		glm::vec2 mouse_coords = glm::min(glm::max(Window::GetMousePos(), glm::vec2(1, 1)), glm::vec2(Window::GetWidth() - 1, Window::GetHeight() - 1));

		uint32_t* pixels = new uint32_t[2];
		glReadPixels(mouse_coords.x, Window::GetHeight() - mouse_coords.y, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, pixels);
		uint64_t current_entity_id = ((uint64_t)pixels[0] << 32) | pixels[1];
		delete[] pixels;

		if (!Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL))
			m_selected_entity_ids.clear();

		SelectEntity(current_entity_id);
	}




	void EditorLayer::DoSelectedEntityHighlightPass() {
		for (auto id : m_selected_entity_ids) {
			auto* current_entity = m_active_scene->GetEntity(id);

			if (!current_entity || !current_entity->HasComponent<MeshComponent>())
				continue;

			MeshComponent* meshc = current_entity->GetComponent<MeshComponent>();

			mp_editor_pass_fb->Bind();
			mp_highlight_shader->ActivateProgram();

			mp_highlight_shader->SetUniform("transform", meshc->GetEntity()->GetComponent<TransformComponent>()->GetMatrix());
			glDisable(GL_DEPTH_TEST);
			Renderer::DrawMeshInstanced(meshc->GetMeshData(), 1);
			glEnable(GL_DEPTH_TEST);
		}

	}




	void EditorLayer::RenderGrid() {
		GL_StateManager::BindSSBO(m_grid_mesh->m_ssbo_handle, GL_StateManager::SSBO_BindingPoints::TRANSFORMS);
		m_grid_mesh->CheckBoundary(mp_editor_camera->GetComponent<TransformComponent>()->GetPosition());
		mp_grid_shader->ActivateProgram();
		Renderer::DrawVAO_ArraysInstanced(GL_LINES, m_grid_mesh->m_vao, ceil(m_grid_mesh->grid_width / m_grid_mesh->grid_step) * 2);
	}




	void EditorLayer::ShowAssetManager() {


		static std::string error_message = "";
		static std::string path = "./res/meshes";

		int window_height = glm::clamp(static_cast<int>(Window::GetHeight()) / 4, 100, 500);
		int window_width = Window::GetWidth() - 800;
		ImVec2 delete_button_size{ 50, 50 };
		ImVec2 button_size = { glm::clamp(window_width / 8.f, 75.f, 150.f) , 150 };
		int column_count = 6;
		ImGui::SetNextWindowSize(ImVec2(window_width, window_height));
		ImGui::SetNextWindowPos(ImVec2(400, Window::GetHeight() - window_height));
		ImGui::GetStyle().CellPadding = ImVec2(11.f, 15.f);

		ImGui::Begin("Assets", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
		ImGui::BeginTabBar("Selection");


		if (ImGui::BeginTabItem("Meshes")) // MESH TAB
		{
			if (ImGui::Button("Add mesh")) // MESH FILE EXPLORER
			{
				// error message display
				ImGui::TextColored(ImVec4(1, 0, 0, 1), error_message.c_str());

				wchar_t valid_extensions[MAX_PATH] = L"Mesh Files: *.obj;*.fbx\0*.obj;*.fbx\0";

				//setting up file explorer callbacks
				std::function<void()> success_callback = [this] {
					MeshAsset* asset = m_active_scene->CreateMeshAsset(path);

					if (asset->LoadMeshData()) {
						m_active_scene->LoadMeshAssetIntoGPU(asset);
						error_message.clear();
					}
					else {
						m_active_scene->DeleteMeshAsset(asset);
						error_message = "Failed to load mesh asset";
					}

				};


				ShowFileExplorer(path, valid_extensions, success_callback);

			} // END MESH FILE EXPLORER


			if (ImGui::BeginTable("Meshes", 4, ImGuiTableFlags_Borders)) // MESH VIEWING TABLE
			{
				for (auto& data : m_active_scene->m_mesh_assets)
				{
					ImGui::PushID(data);
					ImGui::TableNextColumn();
					ImGui::Text(data->GetFilename().substr(data->GetFilename().find_last_of('/') + 1).c_str());
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 0, 0, 0.5));

					if (ImGui::SmallButton("X"))
					{
						m_active_scene->DeleteMeshAsset(data);
					}

					ImGui::PopStyleColor();
					if (data->m_is_loaded)
						ImGui::TextColored(ImVec4(0, 1, 0, 1), "LOADED");
					else
						ImGui::TextColored(ImVec4(1, 0, 0, 1), "NOT LOADED");

					ImGui::PopID();
				}

				ImGui::EndTable();
				ImGui::EndTabItem();

			} // END MESH VIEWING TABLE
		} //	 END MESH TAB


		if (ImGui::BeginTabItem("Textures")) // TEXTURE TAB
		{

			if (ImGui::Button("Add texture")) {
				ImGui::TextColored(ImVec4(1, 0, 0, 1), error_message.c_str());

				static std::string file_extension = "";
				wchar_t valid_extensions[MAX_PATH] = L"Texture Files: *.png;*.jpg;*.jpeg;*.hdr\0*.png;*.jpg;*.jpeg;*.hdr\0";


				//setting up file explorer callbacks
				static std::function<void()> success_callback = [this] {
					m_current_2d_tex_spec.filepath = path;
					m_active_scene->CreateTexture2DAsset(m_current_2d_tex_spec);
					error_message.clear();
				};


				ShowFileExplorer(path, valid_extensions, success_callback);

			}


			if (ImGui::TreeNode("Texture viewer")) // TEXTURE VIEWING TREE NODE
			{

				// Create table for textures 
				if (ImGui::BeginTable("Textures", column_count, ImGuiTableFlags_Borders | ImGuiTableFlags_PadOuterX)); // TEXTURE VIEWING TABLE
				{
					// Push textures into table 
					for (auto* p_texture : m_active_scene->m_texture_2d_assets)
					{
						ImGui::PushID(p_texture);
						ImGui::TableNextColumn();

						if (ImGui::ImageButton(ImTextureID(p_texture->GetTextureHandle()), button_size)) {
							mp_selected_texture = p_texture;
							m_current_2d_tex_spec = mp_selected_texture->m_spec;
						};

						if (ImGui::IsItemActivated()) {
							mp_dragged_texture = p_texture;
						}


						ImGui::Text(p_texture->m_spec.filepath.substr(p_texture->m_spec.filepath.find_last_of('/') + 1).c_str());

						ImGui::PopID();

					}

					ImGui::EndTable();
				} // END TEXTURE VIEWING TABLE
				ImGui::TreePop();
			}
			ImGui::EndTabItem();
		} // END TEXTURE TAB



		if (ImGui::BeginTabItem("Materials")) { // MATERIAL TAB

			if (ImGui::BeginTable("Material viewer", column_count, ImGuiTableFlags_Borders | ImGuiTableFlags_PadOuterX, ImVec2(window_width, window_height))) { //MATERIAL VIEWING TABLE

				for (auto* p_material : m_active_scene->m_materials) {

					ImGui::TableNextColumn();
					ImGui::PushID(p_material);

					bool deletion_flag = false;
					unsigned int tex_id = p_material->base_color_texture ? p_material->base_color_texture->m_texture_obj : CodedAssets::GetBaseTexture().GetTextureHandle();
					if (ImGui::ImageButton(ImTextureID(tex_id), button_size))
						mp_selected_material = p_material;


					// Deletion popup
					if (p_material->uuid() != Scene::BASE_MATERIAL_ID && ImGui::IsItemHovered() && Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_2)) {
						ImGui::OpenPopup("my_select_popup");
					}

					if (ImGui::BeginPopup("my_select_popup"))
					{
						if (ImGui::Selectable("Delete")) {
							deletion_flag = true;
						}
						if (ImGui::Selectable("Duplicate")) {
							auto* p_new_material = m_active_scene->CreateMaterial();
							*p_new_material = *p_material;
						}
						ImGui::EndPopup();
					}
					// End deletion popup

					if (ImGui::IsItemActive()) {
						if (!ImGui::IsItemHovered())
							mp_dragged_material = p_material;

						mp_selected_material = p_material;
					}

					if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
						mp_dragged_material = nullptr;

					ImGui::Text(p_material->name.c_str());

					if (deletion_flag) {
						m_active_scene->DeleteMaterial(p_material->uuid());
						mp_selected_material = p_material == mp_selected_material ? nullptr : mp_selected_material;
						mp_dragged_material = p_material == mp_dragged_material ? nullptr : mp_dragged_material;
					}

					ImGui::PopID();
				}


				ImGui::EndTable();
			} //END MATERIAL VIEWING TABLE
			ImGui::EndTabItem();
		} //END MATERIAL TAB


		ImGui::GetStyle().CellPadding = ImVec2(4.f, 4.f);
		ImGui::EndTabBar();
		ImGui::End();
	}


	void EditorLayer::RenderTextureEditorSection() {

		if (ImGui::Begin("Texture editor")) {

			if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
				// Hide this window
				mp_selected_texture = nullptr;
				goto window_end;
			}

			ImGui::Text(mp_selected_texture->GetSpec().filepath.c_str());

			const char* wrap_modes[] = { "REPEAT", "CLAMP TO EDGE" };
			const char* filter_modes[] = { "LINEAR", "NEAREST" };
			static int selected_wrap_mode = m_current_2d_tex_spec.wrap_params == GL_REPEAT ? 0 : 1;
			static int selected_filter_mode = m_current_2d_tex_spec.mag_filter == GL_LINEAR ? 0 : 1;

			ImGui::Checkbox("SRGB", &m_current_2d_tex_spec.srgb_space);

			ImGui::Text("Wrap mode");
			ImGui::SameLine();
			ImGui::Combo("##Wrap mode", &selected_wrap_mode, wrap_modes, IM_ARRAYSIZE(wrap_modes));
			m_current_2d_tex_spec.wrap_params = selected_wrap_mode == 0 ? GL_REPEAT : GL_CLAMP_TO_EDGE;

			ImGui::Text("Filtering");
			ImGui::SameLine();
			ImGui::Combo("##Filter mode", &selected_filter_mode, filter_modes, IM_ARRAYSIZE(filter_modes));
			m_current_2d_tex_spec.mag_filter = selected_filter_mode == 0 ? GL_LINEAR : GL_NEAREST;
			m_current_2d_tex_spec.min_filter = selected_filter_mode == 0 ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST;

			m_current_2d_tex_spec.generate_mipmaps = true;

			if (ImGui::Button("Load")) {
				mp_selected_texture->SetSpec(m_current_2d_tex_spec);
				mp_selected_texture->LoadFromFile();
			}
		}

	window_end:
		ImGui::End();
	}



	void EditorLayer::RenderSkyboxEditor() {
		if (H1TreeNode("Skybox")) {
			static std::string hdr_filepath = m_active_scene->skybox.m_hdr_tex_filepath;
			ImGui::InputText("HDR Filepath", &hdr_filepath);

			wchar_t valid_extensions[MAX_PATH] = L"HDR Files: *.hdr\0*.hdr\0";

			std::function<void()> file_explorer_callback = [this] {
				m_active_scene->skybox.LoadEnvironmentMap(hdr_filepath);
			};

			if (ImGui::Button("Load HDR")) {
				ShowFileExplorer(hdr_filepath, valid_extensions, file_explorer_callback);
			}

		}
	}



	void EditorLayer::RenderMaterialEditorSection() {

		if (ImGui::Begin("Material editor")) {

			if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
				// Hide this tree node
				mp_selected_material = nullptr;
				goto window_end;
			}


			ImGui::Text("Name: ");
			ImGui::SameLine();
			ImGui::InputText("##name input", &mp_selected_material->name);
			ImGui::Spacing();

			RenderMaterialTexture("Base", mp_selected_material->base_color_texture);
			RenderMaterialTexture("Normal", mp_selected_material->normal_map_texture);
			RenderMaterialTexture("Roughness", mp_selected_material->roughness_texture);
			RenderMaterialTexture("Metallic", mp_selected_material->metallic_texture);
			RenderMaterialTexture("AO", mp_selected_material->ao_texture);
			RenderMaterialTexture("Displacement", mp_selected_material->displacement_texture);
			RenderMaterialTexture("Emissive", mp_selected_material->emissive_texture);

			ImGui::Text("Colors");
			ImGui::Spacing();
			ShowVec3Editor("Base color", mp_selected_material->base_color);

			if (!mp_selected_material->roughness_texture)
				ImGui::SliderFloat("Roughness", &mp_selected_material->roughness, 0.f, 1.f);

			if (!mp_selected_material->metallic_texture)
				ImGui::SliderFloat("Metallic", &mp_selected_material->metallic, 0.f, 1.f);

			if (!mp_selected_material->ao_texture)
				ImGui::SliderFloat("AO", &mp_selected_material->ao, 0.f, 1.f);

			ImGui::Checkbox("Emissive", &mp_selected_material->emissive);

			if (mp_selected_material->emissive || mp_selected_material->emissive_texture)
				ImGui::SliderFloat("Emissive strength", &mp_selected_material->emissive_strength, -10.f, 10.f);

			int num_parallax_layers = mp_selected_material->parallax_layers;
			if (mp_selected_material->displacement_texture) {
				ImGui::InputInt("Parallax layers", &num_parallax_layers);

				if (num_parallax_layers >= 0)
					mp_selected_material->parallax_layers = num_parallax_layers;

				ImGui::InputFloat("Parallax scale", &mp_selected_material->parallax_height_scale);
			}

			ShowVec2Editor("Tile scale", mp_selected_material->tile_scale);
		}

	window_end:
		if (!Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_1))
			// Reset drag if mouse not held down
			mp_dragged_texture = nullptr;

		ImGui::End();
	}




	void EditorLayer::RenderEntityNode(SceneEntity* p_entity) {

		// Setup display name with icons
		static std::string formatted_name; // Static to stop a new string being made every single call, only needed for c_str anyway
		formatted_name.clear();

		if (ImGui::IsItemVisible()) {
			formatted_name += p_entity->HasComponent<MeshComponent>() ? " " ICON_FA_BOX : "";
			formatted_name += p_entity->HasComponent<PhysicsComponent>() ? " " ICON_FA_WIND : "";
			formatted_name += p_entity->HasComponent<PointLightComponent>() ? " " ICON_FA_LIGHTBULB : "";
			formatted_name += p_entity->HasComponent<SpotLightComponent>() ? " " ICON_FA_LIGHTBULB : "";
			formatted_name += p_entity->HasComponent<CameraComponent>() ? " " ICON_FA_CAMERA : "";
			formatted_name += " ";
			formatted_name += p_entity->name;
			if (!p_entity->m_children.empty())
				formatted_name += " (" + std::to_string(p_entity->m_children.size()) + ")";
		}

		ImVec4 tree_node_bg_col = VectorContains(m_selected_entity_ids, p_entity->GetUUID()) ? lighter_grey_color : ImVec4(0, 0, 0, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_Header, tree_node_bg_col);
		// Push unique ID
		ImGui::PushID(p_entity);
		// OPEN TREE NODE
		// Tree node opened this way to make some styling logic possible below
		auto flags = p_entity->m_children.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Framed : ImGuiTreeNodeFlags_Framed;
		bool is_tree_node_open = ImGui::TreeNodeEx(formatted_name.c_str(), flags);
		//
		// 
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		std::string popup_id = std::format("{}", p_entity->GetUUID()); // unique popup id

		m_selected_entities_are_dragged |= ImGui::IsItemActive() && !ImGui::IsItemHovered();

		if (ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax())) {
			// Drag entities into another entity node to make them children of it
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && m_selected_entities_are_dragged) {
				DeselectEntity(p_entity->GetUUID());
				for (auto id : m_selected_entity_ids) {
					m_active_scene->GetEntity(id)->SetParent(p_entity);
				}

				m_selected_entities_are_dragged = false;
			}

			// Right click to open popup 
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				ImGui::OpenPopup(popup_id.c_str());

		}
		// If clicked, select current entity
		if (ImGui::IsItemHovered() && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) { // Doing this instead of IsActive() because IsActive wont trigger if ctrl is held down
			if (!Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL)) // Only selecting one entity at a time
				m_selected_entity_ids.clear();


			if (Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL) && VectorContains(m_selected_entity_ids, p_entity->GetUUID())) // Deselect entity from group of entities currently selected
				m_selected_entity_ids.erase(std::ranges::find(m_selected_entity_ids, p_entity->GetUUID()));
			else
				SelectEntity(p_entity->GetUUID());

		}

		// Popup opened above if node is right clicked
		if (ImGui::BeginPopup(popup_id.c_str()))
		{

			ImGui::SeparatorText("Options");
			if (ImGui::Selectable("Delete")) {

				for (auto id : m_selected_entity_ids) {
					m_active_scene->DeleteEntity(m_active_scene->GetEntity(id));
				}
				m_selected_entity_ids.clear();

				// Early return as logic below will now rely on a deleted entity
				ImGui::EndPopup();
				goto exit;
			}


			// Creates clone of entity, puts it in scene
			if (ImGui::Selectable("Duplicate")) {
				m_selected_entity_ids.clear(); // Clear here so only the new duplicates are selected, not the old originals.
				for (auto id : m_selected_entity_ids) {
					DuplicateEntity(m_active_scene->GetEntity(id));
				}
			}

			ImGui::EndPopup();
		}

		// Render entity nodes for all the children of this entity
		if (p_entity && is_tree_node_open) {
			for (auto* p_child : p_entity->m_children) {
				RenderEntityNode(p_child);
			}
		}

	exit:
		if (is_tree_node_open)
			ImGui::TreePop(); // Pop tree node opened earlier

		ImGui::PopID();
		return;
	}



	void EditorLayer::RenderSceneGraph() {
		if (ImGui::Begin("Scene graph")) {

			// Right click to bring up "new entity" popup
			RenderCreationWidget(nullptr, ImGui::IsWindowHovered() && Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_2));

			if (ImGui::Button("Save")) {
				SceneSerializer::SerializeScene(*m_active_scene, "scene.yml");
			}
			if (ImGui::Button("Load")) {
				mp_dragged_material = nullptr;
				mp_selected_material = nullptr;
				mp_selected_texture = nullptr;
				mp_dragged_texture = nullptr;
				m_selected_entity_ids.clear();
				bool saved_physics_state = m_active_scene->m_physics_system.GetIsPaused();

				auto camera_saved_transforms = mp_editor_camera->GetComponent<TransformComponent>()->GetAbsoluteTransforms();
				glm::vec3 cam_pos = camera_saved_transforms[0];
				glm::vec3 cam_orientation = camera_saved_transforms[2];
				m_active_scene->m_physics_system.SetIsPaused(true);

				m_active_scene->UnloadScene();
				m_active_scene->LoadScene("scene.yml");
				mp_editor_camera = std::make_unique<SceneEntity>(&*m_active_scene, m_active_scene->m_registry.create());
				auto* p_transform = mp_editor_camera->AddComponent<TransformComponent>();
				p_transform->SetAbsolutePosition(cam_pos);
				p_transform->SetAbsoluteOrientation(cam_orientation);
				mp_editor_camera->AddComponent<EditorCamera>();
				mp_editor_camera->GetComponent<EditorCamera>()->MakeActive();

				m_active_scene->m_physics_system.SetIsPaused(saved_physics_state);
			}

			bool physics_paused = m_active_scene->m_physics_system.GetIsPaused();
			if (ImGui::RadioButton("Physics", !physics_paused))
				m_active_scene->m_physics_system.SetIsPaused(!physics_paused);

			ImGui::Text("Editor cam exposure");
			ImGui::SliderFloat("##exposure", &mp_editor_camera->GetComponent<EditorCamera>()->exposure, 0.f, 10.f);


			if (H2TreeNode("Entities")) {

				m_display_skybox_editor = EmptyTreeNode("Skybox");
				m_display_directional_light_editor = EmptyTreeNode("Directional Light");
				m_display_global_fog_editor = EmptyTreeNode("Global fog");
				m_display_terrain_editor = EmptyTreeNode("Terrain");
				m_display_bloom_editor = EmptyTreeNode("Bloom");

				for (auto* p_entity : m_active_scene->m_entities) {
					if (p_entity->GetParent())
						continue;

					RenderEntityNode(p_entity);
				}
			}


			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				if (m_selected_entities_are_dragged && ImGui::IsWindowHovered()) {
					for (auto id : m_selected_entity_ids) {
						m_active_scene->GetEntity(id)->SetParent(nullptr);
					}
				}

				m_selected_entities_are_dragged = false;
			}


		} // begin "scene graph"

		ImGui::End();
	}


	void EditorLayer::DisplayEntityEditor() {
		if (ImGui::Begin("Entity editor")) {

			if (m_display_directional_light_editor)
				RenderDirectionalLightEditor();
			if (m_display_global_fog_editor)
				RenderGlobalFogEditor();
			if (m_display_skybox_editor)
				RenderSkyboxEditor();
			if (m_display_terrain_editor)
				RenderTerrainEditor();
			if (m_display_bloom_editor)
				RenderBloomEditor();

			auto entity = m_active_scene->GetEntity(m_selected_entity_ids.empty() ? 0 : m_selected_entity_ids[0]);
			if (!entity) {
				ImGui::End();
				return;
			}

			auto meshc = entity->HasComponent<MeshComponent>() ? entity->GetComponent<MeshComponent>() : nullptr;
			auto plight = entity->HasComponent<PointLightComponent>() ? entity->GetComponent<PointLightComponent>() : nullptr;
			auto slight = entity->HasComponent<SpotLightComponent>() ? entity->GetComponent<SpotLightComponent>() : nullptr;
			auto p_cam = entity->HasComponent<CameraComponent>() ? entity->GetComponent<CameraComponent>() : nullptr;
			auto p_physics_comp = entity->HasComponent<PhysicsComponent>() ? entity->GetComponent<PhysicsComponent>() : nullptr;

			std::vector<TransformComponent*> transforms;
			for (auto id : m_selected_entity_ids) {
				transforms.push_back(m_active_scene->GetEntity(id)->GetComponent<TransformComponent>());
			}
			std::string ent_text = std::format("Entity '{}'", entity->name);
			ImGui::InputText("Name", &entity->name);
			ImGui::Text(ent_text.c_str());


			//TRANSFORM
			if (H2TreeNode("Entity transform")) {
				RenderTransformComponentEditor(transforms);
			}


			//MESH
			if (meshc) {

				if (H2TreeNode("Mesh component")) {
					ImGui::Text("Mesh asset name");
					ImGui::SameLine();
					static std::string input_filename;
					ImGui::InputText("##filename input", &input_filename);

					if (ImGui::Button("Link asset")) {

						for (auto p_mesh_asset : m_active_scene->m_mesh_assets) {
							std::string mesh_filename = p_mesh_asset->GetFilename().substr(p_mesh_asset->GetFilename().find_last_of('/') + 1);
							std::transform(mesh_filename.begin(), mesh_filename.end(), mesh_filename.begin(), ::toupper);
							std::transform(input_filename.begin(), input_filename.end(), input_filename.begin(), ::toupper);

							if (mesh_filename == input_filename) {
								meshc->SetMeshAsset(p_mesh_asset);
							}

						}
					};

					if (meshc->mp_mesh_asset) {
						RenderMeshComponentEditor(meshc);
					}
				}

			}


			ImGui::PushID(plight);
			if (plight && H2TreeNode("Pointlight component")) {
				if (ImGui::Button("DELETE")) {
					entity->DeleteComponent<PointLightComponent>();
				}
				else {
					RenderPointlightEditor(plight);
				}
			}
			ImGui::PopID();

			ImGui::PushID(slight);
			if (slight && H2TreeNode("Spotlight component")) {
				if (ImGui::Button("DELETE")) {
					entity->DeleteComponent<SpotLightComponent>();
				}
				else {
					RenderSpotlightEditor(slight);
				}
			}
			ImGui::PopID();

			ImGui::PushID(p_cam);
			if (p_cam && H2TreeNode("Camera component")) {
				if (ImGui::Button("DELETE")) {
					entity->DeleteComponent<CameraComponent>();
				}
				RenderCameraEditor(p_cam);
			}
			ImGui::PopID();

			ImGui::PushID(p_physics_comp);
			if (p_physics_comp && H2TreeNode("Physics component")) {
				if (ImGui::Button("DELETE")) {
					entity->DeleteComponent<PhysicsComponent>();
				}
				else {
					RenderPhysicsComponentEditor(p_physics_comp);
				}
			}
			ImGui::PopID();

			glm::vec2 window_size = { ImGui::GetWindowSize().x, ImGui::GetWindowSize().y };
			glm::vec2 button_size = { 200, 50 };
			glm::vec2 padding_size = { (window_size.x / 2.f) - button_size.x / 2.f, 50.f };
			ImGui::Dummy(ImVec2(padding_size.x, padding_size.y));

			RenderCreationWidget(entity, ImGui::Button("Add component", ImVec2(button_size.x, button_size.y)));

		}
		ImGui::End(); // end entity editor window
	}



	// EDITORS ------------------------------------------------------------------------

	void EditorLayer::RenderPhysicsMaterial(physx::PxMaterial* p_material) {
		float restitution = p_material->getRestitution();
		if (ClampedFloatInput("Restitution", &restitution, 0.f, 1.f)) {
			p_material->setRestitution(restitution);
		}

		float dynamic_friction = p_material->getDynamicFriction();
		if (ClampedFloatInput("Dynamic friction", &dynamic_friction, 0.f, 1.f)) {
			p_material->setDynamicFriction(dynamic_friction);
		}

		float static_friction = p_material->getStaticFriction();
		if (ClampedFloatInput("Static friction", &static_friction, 0.f, 1.f)) {
			p_material->setStaticFriction(static_friction);
		}
	}




	void EditorLayer::RenderPhysicsComponentEditor(PhysicsComponent* p_comp) {
		ImGui::SeparatorText("Collider geometry");

		if (ImGui::RadioButton("Box", p_comp->geometry_type == PhysicsComponent::BOX)) {
			p_comp->UpdateGeometry(PhysicsComponent::BOX);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Sphere", p_comp->geometry_type == PhysicsComponent::SPHERE)) {
			p_comp->UpdateGeometry(PhysicsComponent::SPHERE);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Mesh", p_comp->geometry_type == PhysicsComponent::TRIANGLE_MESH)) {
			p_comp->UpdateGeometry(PhysicsComponent::TRIANGLE_MESH);
		}


		ImGui::SeparatorText("Collider behaviour"); // Currently don't support dynamic triangle meshes
		if (ImGui::RadioButton("Dynamic", p_comp->rigid_body_type == PhysicsComponent::DYNAMIC)) {
			p_comp->SetBodyType(PhysicsComponent::DYNAMIC);
		}
		if (ImGui::RadioButton("Static", p_comp->rigid_body_type == PhysicsComponent::STATIC)) {
			p_comp->SetBodyType(PhysicsComponent::STATIC);
		}


		RenderPhysicsMaterial(p_comp->p_material);

	}




	void EditorLayer::RenderTransformComponentEditor(std::vector<TransformComponent*>& transforms) {


		static bool render_gizmos = true;

		ImGui::Checkbox("Gizmos", &render_gizmos);

		static bool absolute_mode = false;

		if (ImGui::Checkbox("Absolute", &absolute_mode)) {
			transforms[0]->SetAbsoluteMode(absolute_mode);
		}

		glm::vec3 matrix_translation = transforms[0]->m_pos;
		glm::vec3 matrix_rotation = transforms[0]->m_orientation;
		glm::vec3 matrix_scale = transforms[0]->m_scale;

		// UI section
		if (ShowVec3Editor("Tr", matrix_translation))
			std::ranges::for_each(transforms, [matrix_translation](TransformComponent* p_transform) {p_transform->SetPosition(matrix_translation); });

		if (ShowVec3Editor("Rt", matrix_rotation))
			std::ranges::for_each(transforms, [matrix_rotation](TransformComponent* p_transform) {p_transform->SetOrientation(matrix_rotation); });

		if (ShowVec3Editor("Sc", matrix_scale))
			std::ranges::for_each(transforms, [matrix_scale](TransformComponent* p_transform) {p_transform->SetScale(matrix_scale); });


		if (!render_gizmos)
			return;

		// Gizmos 
		ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::BeginFrame();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

		static ImGuizmo::OPERATION current_operation = ImGuizmo::TRANSLATE;
		static ImGuizmo::MODE current_mode = ImGuizmo::WORLD;


		if (Window::IsKeyDown(GLFW_KEY_1))
			current_operation = ImGuizmo::TRANSLATE;
		else if (Window::IsKeyDown(GLFW_KEY_2))
			current_operation = ImGuizmo::SCALE;
		else if (Window::IsKeyDown(GLFW_KEY_3))
			current_operation = ImGuizmo::ROTATE;

		ImGui::Text("Gizmo rendering");
		if (ImGui::RadioButton("World", current_mode == ImGuizmo::WORLD))
			current_mode = ImGuizmo::WORLD;
		ImGui::SameLine();
		if (ImGui::RadioButton("Local", current_mode == ImGuizmo::LOCAL))
			current_mode = ImGuizmo::LOCAL;

		glm::mat4 current_operation_matrix = transforms[0]->GetMatrix();

		static glm::mat4 delta_matrix;
		CameraComponent* p_cam = m_active_scene->m_camera_system.GetActiveCamera();
		if (!p_cam)
			p_cam = static_cast<CameraComponent*>(mp_editor_camera->GetComponent<EditorCamera>());

		if (ImGuizmo::Manipulate(&p_cam->GetViewMatrix()[0][0], &p_cam->GetProjectionMatrix()[0][0], current_operation, current_mode, &current_operation_matrix[0][0], &delta_matrix[0][0], nullptr) && ImGuizmo::IsUsing()) {

			ImGuizmo::DecomposeMatrixToComponents(&delta_matrix[0][0], &matrix_translation[0], &matrix_rotation[0], &matrix_scale[0]);

			auto base_abs_transforms = transforms[0]->GetAbsoluteTransforms();
			glm::vec3 base_abs_translation = base_abs_transforms[0];
			glm::vec3 base_abs_scale = base_abs_transforms[1];
			glm::vec3 base_abs_rotation = base_abs_transforms[2];

			glm::vec3 delta_translation = matrix_translation;
			glm::vec3 delta_scale = matrix_scale;
			glm::vec3 delta_rotation = matrix_rotation;

			for (auto* p_transform : transforms) {

				auto current_transforms = p_transform->GetAbsoluteTransforms();
				switch (current_operation) {
				case ImGuizmo::TRANSLATE:
					p_transform->SetPosition(p_transform->GetPosition() + delta_translation);
					break;
				case ImGuizmo::SCALE:
					p_transform->SetScale(p_transform->GetScale() * delta_scale);
					break;
				case ImGuizmo::ROTATE: // This will rotate multiple objects as one, using entity transform at m_selected_entity_ids[0] as origin
					glm::vec3 abs_rotation = current_transforms[2];
					glm::vec3 inherited_rotation = abs_rotation - p_transform->GetOrientation();
					glm::mat4 new_rotation_mat = ExtraMath::Init3DRotateTransform(delta_rotation.x, delta_rotation.y, delta_rotation.z) * ExtraMath::Init3DRotateTransform(abs_rotation.x, abs_rotation.y, abs_rotation.z);
					ImGuizmo::DecomposeMatrixToComponents(&new_rotation_mat[0][0], &matrix_translation[0], &matrix_rotation[0], &matrix_scale[0]);
					p_transform->SetOrientation(matrix_rotation);

					glm::vec3 abs_translation = current_transforms[0];
					glm::vec3 transformed_pos = abs_translation - base_abs_translation;
					glm::vec3 rotation_offset = glm::mat3(ExtraMath::Init3DRotateTransform(delta_rotation.x, delta_rotation.y, delta_rotation.z)) * transformed_pos; // rotate around transformed origin
					p_transform->SetAbsolutePosition(base_abs_translation + rotation_offset);
					break;
				}

			}
		};

	}


	Material* EditorLayer::RenderMaterialComponent(const Material* p_material) {
		Material* ret = nullptr;
		if (mp_dragged_material) { // Highlight as drag-drop target
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 1));
		}

		if (ImGui::ImageButton(ImTextureID(p_material->base_color_texture ? p_material->base_color_texture->GetTextureHandle() : CodedAssets::GetBaseTexture().GetTextureHandle()), ImVec2(100, 100))) {
			mp_selected_material = m_active_scene->GetMaterial(p_material->uuid());
		};

		if (ImGui::IsItemHovered() && mp_dragged_material) {
			ret = mp_dragged_material;
			ImGui::PopStyleColor();
			mp_dragged_material = nullptr;
		}

		if (mp_dragged_material) {
			ImGui::PopStyleColor();
		}

		ImGui::Text(p_material->name.c_str());
		return ret;
	}


	void EditorLayer::RenderMeshComponentEditor(MeshComponent* comp) {

		ImGui::PushID(comp);
		for (int i = 0; i < comp->m_materials.size(); i++) {
			auto p_material = comp->m_materials[i];
			ImGui::PushID(i);

			if (Material* p_new_material = RenderMaterialComponent(p_material)) {
				// Needs to be re-sorted as the material has changed
				comp->SetMaterialID(i, p_new_material);
			}

			ImGui::PopID();
		}


		ImGui::PopID();
	};



	void EditorLayer::RenderSpotlightEditor(SpotLightComponent* light) {

		float aperture = glm::degrees(acosf(light->m_aperture));
		glm::vec3 dir = light->m_light_direction_vec;

		ImGui::PushItemWidth(200.f);
		ImGui::SliderFloat("constant", &light->attenuation.constant, 0.0f, 1.0f);
		ImGui::SliderFloat("linear", &light->attenuation.linear, 0.0f, 1.0f);
		ImGui::SliderFloat("exp", &light->attenuation.exp, 0.0f, 0.1f);
		ImGui::Checkbox("Shadows", &light->shadows_enabled);
		ImGui::SliderFloat("Shadow distance", &light->shadow_distance, 0.0f, 5000.0f);

		ImGui::Text("Aperture");
		ImGui::SameLine();
		ImGui::SliderFloat("##a", &aperture, 0.f, 90.f);

		ImGui::PopItemWidth();

		ShowVec3Editor("Direction", dir);
		ShowColorVec3Editor("Color", light->color);

		light->SetLightDirection(dir.x, dir.y, dir.z);
		light->SetAperture(aperture);
	}


	void EditorLayer::RenderGlobalFogEditor() {
		if (H2TreeNode("Global fog")) {
			ImGui::Text("Scattering");
			ImGui::SliderFloat("##scattering", &m_active_scene->post_processing.global_fog.scattering_coef, 0.f, 0.1f);
			ImGui::Text("Absorption");
			ImGui::SliderFloat("##absorption", &m_active_scene->post_processing.global_fog.absorption_coef, 0.f, 0.1f);
			ImGui::Text("Density");
			ImGui::SliderFloat("##density", &m_active_scene->post_processing.global_fog.density_coef, 0.f, 1.f);
			ImGui::Text("Scattering anistropy");
			ImGui::SliderFloat("##scattering anistropy", &m_active_scene->post_processing.global_fog.scattering_anistropy, -1.f, 1.f);
			ImGui::Text("Emissive factor");
			ImGui::SliderFloat("##emissive", &m_active_scene->post_processing.global_fog.emissive_factor, 0.f, 2.f);
			ImGui::Text("Step count");
			ImGui::SliderInt("##step count", &m_active_scene->post_processing.global_fog.step_count, 0, 512);
			ShowColorVec3Editor("Color", m_active_scene->post_processing.global_fog.color);
		}
	}




	void EditorLayer::RenderBloomEditor() {
		if (H2TreeNode("Bloom")) {
			ImGui::SliderFloat("Intensity", &m_active_scene->post_processing.bloom.intensity, 0.f, 10.f);
			ImGui::SliderFloat("Threshold", &m_active_scene->post_processing.bloom.threshold, 0.0f, 50.0f);
			ImGui::SliderFloat("Knee", &m_active_scene->post_processing.bloom.knee, 0.f, 1.f);
		}
	}



	void EditorLayer::RenderTerrainEditor() {
		if (H2TreeNode("Terrain")) {
			ImGui::InputFloat("Height factor", &m_active_scene->terrain.m_height_scale);
			static int terrain_width = 1000;

			if (ImGui::InputInt("Size", &terrain_width) && terrain_width > 500)
				m_active_scene->terrain.m_width = terrain_width;
			else
				terrain_width = m_active_scene->terrain.m_width;

			static int terrain_seed = 123;
			ImGui::InputInt("Seed", &terrain_seed);
			m_active_scene->terrain.m_seed = terrain_seed;

			if (Material* p_new_material = RenderMaterialComponent(m_active_scene->terrain.mp_material))
				m_active_scene->terrain.mp_material = p_new_material;

			if (ImGui::Button("Reload"))
				m_active_scene->terrain.ResetTerrainQuadtree();

		}
	}

	void EditorLayer::RenderDirectionalLightEditor() {
		if (H1TreeNode("Directional light")) {
			ImGui::Text("DIR LIGHT CONTROLS");

			static glm::vec3 light_dir = glm::vec3(0, 0.5, 0.5);
			static glm::vec3 light_color = m_active_scene->m_directional_light.color;

			ImGui::SliderFloat("X", &light_dir.x, -1.f, 1.f);
			ImGui::SliderFloat("Y", &light_dir.y, -1.f, 1.f);
			ImGui::SliderFloat("Z", &light_dir.z, -1.f, 1.f);
			ShowColorVec3Editor("Color", light_color);

			ImGui::Text("Cascade ranges");
			ImGui::SliderFloat("##c1", &m_active_scene->m_directional_light.cascade_ranges[0], 0.f, 50.f);
			ImGui::SliderFloat("##c2", &m_active_scene->m_directional_light.cascade_ranges[1], 0.f, 150.f);
			ImGui::SliderFloat("##c3", &m_active_scene->m_directional_light.cascade_ranges[2], 0.f, 500.f);
			ImGui::Text("Z-mults");
			ImGui::SliderFloat("##z1", &m_active_scene->m_directional_light.z_mults[0], 0.f, 10.f);
			ImGui::SliderFloat("##z2", &m_active_scene->m_directional_light.z_mults[1], 0.f, 10.f);
			ImGui::SliderFloat("##z3", &m_active_scene->m_directional_light.z_mults[2], 0.f, 10.f);

			ImGui::SliderFloat("Size", &m_active_scene->m_directional_light.light_size, 0.f, 150.f);
			ImGui::SliderFloat("Blocker search size", &m_active_scene->m_directional_light.blocker_search_size, 0.f, 50.f);

			m_active_scene->m_directional_light.color = glm::vec3(light_color.x, light_color.y, light_color.z);
			m_active_scene->m_directional_light.SetLightDirection(light_dir);
		}
	}


	void EditorLayer::RenderPointlightEditor(PointLightComponent* light) {

		ImGui::PushItemWidth(200.f);
		ImGui::SliderFloat("constant", &light->attenuation.constant, 0.0f, 1.0f);
		ImGui::SliderFloat("linear", &light->attenuation.linear, 0.0f, 1.0f);
		ImGui::SliderFloat("exp", &light->attenuation.exp, 0.0f, 0.1f);
		ImGui::Checkbox("Shadows", &light->shadows_enabled);
		ImGui::SliderFloat("Shadow distance", &light->shadow_distance, 0.0f, 5000.0f);

		ImGui::PopItemWidth();
		ShowColorVec3Editor("Color", light->color);
	}

	void EditorLayer::RenderCameraEditor(CameraComponent* p_cam) {
		ImGui::PushItemWidth(200.f);
		ImGui::SliderFloat("Exposure", &p_cam->exposure, 0.f, 10.f);
		ImGui::SliderFloat("FOV", &p_cam->fov, 0.f, 180.f);
		ImGui::InputFloat("ZNEAR", &p_cam->zNear);
		ImGui::InputFloat("ZFAR", &p_cam->zFar);
		ShowVec3Editor("Up", p_cam->up);

		if (ImGui::Button("Make active")) {
			p_cam->MakeActive();
		}
	}



	bool EditorLayer::ShowVec3Editor(const char* name, glm::vec3& vec, float min, float max) {
		bool ret = false;
		glm::vec3 vec_copy = vec;
		ImGui::PushID(&vec);
		ImGui::Text(name);
		ImGui::PushItemWidth(100.f);
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
		ImGui::SameLine();

		if (ImGui::InputFloat("##x", &vec_copy.x) && vec_copy.x > min && vec_copy.x < max) {
			vec.x = vec_copy.x;
			ret = true;
		}


		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y");
		ImGui::SameLine();

		if (ImGui::InputFloat("##y", &vec_copy.y) && vec_copy.y > min && vec_copy.y < max) {
			vec.y = vec_copy.y;
			ret = true;
		}

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z");
		ImGui::SameLine();

		if (ImGui::InputFloat("##z", &vec_copy.z) && vec_copy.z > min && vec_copy.z < max) {
			vec.z = vec_copy.z;
			ret = true;
		}

		ImGui::PopItemWidth();
		ImGui::PopID();

		return ret;
	}

	bool EditorLayer::ShowVec2Editor(const char* name, glm::vec2& vec, float min, float max) {
		bool ret = false;
		glm::vec2 vec_copy = vec;
		ImGui::PushID(&vec);
		ImGui::Text(name);
		ImGui::PushItemWidth(100.f);
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
		ImGui::SameLine();

		if (ImGui::InputFloat("##x", &vec_copy.x) && vec_copy.x > min && vec_copy.x < max) {
			vec.x = vec_copy.x;
			ret = true;
		}


		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y");
		ImGui::SameLine();

		if (ImGui::InputFloat("##y", &vec_copy.y) && vec_copy.y > min && vec_copy.y < max) {
			vec.y = vec_copy.y;
			ret = true;
		}

		ImGui::PopItemWidth();
		ImGui::PopID();

		return ret;
	}



	void EditorLayer::ShowFileExplorer(std::string& path_name, wchar_t extension_filter[], std::function<void()> valid_file_callback) {
		// Create an OPENFILENAMEW structure
		// Create an OPENFILENAMEW structure
		OPENFILENAMEW ofn;
		wchar_t fileNames[MAX_PATH * 100] = { 0 };

		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFile = fileNames;
		ofn.lpstrFilter = extension_filter;
		ofn.nMaxFile = sizeof(fileNames);
		ofn.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT;

		// This needs to be stored to keep relative filepaths working, otherwise the working directory will be changed
		std::filesystem::path prev_path{std::filesystem::current_path().generic_string()};
		// Display the File Open dialog
		if (GetOpenFileNameW(&ofn))
		{
			// Process the selected files
			std::wstring folderPath = fileNames;

			// Get the length of the folder path
			size_t folderPathLen = folderPath.length();

			// Pointer to the first character after the folder path
			wchar_t* currentFileName = fileNames + folderPathLen + 1;
			bool single_file = currentFileName[0] == '\0';

			int max_safety_iterations = 5000;
			int i = 0;
			// Loop through the selected files
			while (i < max_safety_iterations && (*currentFileName || single_file))
			{
				i++;
				// Construct the full file path
				std::wstring filePath = single_file ? folderPath : folderPath + L"\\" + currentFileName;

				path_name = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(filePath);
				std::ranges::for_each(path_name, [](char& c) { c = c == '\\' ? '/' : c; });
				path_name.erase(0, path_name.find("/res/"));
				path_name = "." + path_name;

				// Reset path to stop relative paths breaking
				std::filesystem::current_path(prev_path);
				valid_file_callback();

				// Move to the next file name
				currentFileName += wcslen(currentFileName) + 1;
				single_file = false;
			}



		}

	}




	bool EditorLayer::ShowColorVec3Editor(const char* name, glm::vec3& vec) {
		bool ret = false;
		ImGui::PushID(&vec);
		ImGui::Text(name);
		ImGui::PushItemWidth(100.f);

		ImGui::TextColored(ImVec4(1, 0, 0, 1), "R");
		ImGui::SameLine();

		if (ImGui::InputFloat("##r", &vec.x))
			ret = true;

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "G");
		ImGui::SameLine();

		if (ImGui::InputFloat("##g", &vec.y))
			ret = true;

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 0, 1, 1), "B");
		ImGui::SameLine();

		if (ImGui::InputFloat("##b", &vec.z))
			ret = true;

		ImGui::PopItemWidth();
		ImGui::PopID();

		return ret;
	}




	void EditorLayer::RenderMaterialTexture(const char* name, Texture2D*& p_tex) {
		ImGui::PushID(p_tex);
		if (p_tex) {
			ImGui::Text(std::format("{} texture - {}", name, p_tex->m_name).c_str());
			if (ImGui::ImageButton(ImTextureID(p_tex->GetTextureHandle()), ImVec2(75, 75))) {
				mp_selected_texture = p_tex;
				m_current_2d_tex_spec = p_tex->m_spec;
			};
		}
		else {
			ImGui::Text(std::format("{} texture - NONE", name).c_str());
			ImGui::ImageButton(ImTextureID(0), ImVec2(75, 75));
		}

		if (ImGui::IsItemHovered()) {
			if (mp_dragged_texture)
				p_tex = mp_dragged_texture;

			if (Window::IsMouseButtonDown(GLFW_MOUSE_BUTTON_2))
				p_tex = nullptr;
		}

		ImGui::PopID();
	}

	bool EditorLayer::ClampedFloatInput(const char* name, float* p_val, float min, float max) {
		float val = *p_val;
		bool r = false;
		if (ImGui::InputFloat(name, &val) && val <= max && val >= min) {
			*p_val = val;
			r = true;
		}
		return r;
	}

	bool EditorLayer::H1TreeNode(const char* name) {
		float original_size = ImGui::GetFont()->Scale;
		ImGui::GetFont()->Scale *= 1.15f;
		ImGui::PushFont(ImGui::GetFont());
		bool r = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::GetFont()->Scale = original_size;
		ImGui::PopFont();
		return r;
	}

	bool EditorLayer::H2TreeNode(const char* name) {
		float original_size = ImGui::GetFont()->Scale;
		ImGui::GetFont()->Scale *= 1.1f;
		ImGui::PushFont(ImGui::GetFont());
		bool r = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::GetFont()->Scale = original_size;
		ImGui::PopFont();
		return r;
	}

	bool EditorLayer::EmptyTreeNode(const char* name) {
		bool ret = ImGui::TreeNode(name);
		if (ret)
			ImGui::TreePop();

		return ret;
	}

}