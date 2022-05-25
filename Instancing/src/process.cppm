#include <logging.hxx>;
import <thread>;
import <chrono>;

import MainApplication.Instancing;

import <GLFW/glfw3.h>;
import <glm/gtc/matrix_transform.hpp>;
import Helpers.GLFW;

import Core.Events;

// already very clean
bool Instancing::Context::KeepContextRunning () {
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	return !glfwWindowShouldClose(MainWindow);
}


float FITNESS_FUNCTION(glm::vec3 in) {
	return in.x*in.x + in.y*in.y + in.z*in.z + 11;
}

void Instancing::Instance::setup (const std::span<char*> &argument_list) { 
	LOG_trace(__FUNCSIG__);

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	auto framebuffer_resize_callback = [](GLFWwindow* window, int width, int height) {
			auto* app_instance = (Instancing::Instance*)(glfwGetWindowUserPointer(window));
			app_instance->recreate_swapchain_and_related ();
		};

	if (m_Context.MainWindow == nullptr){
		m_Context.MainWindow = glfwCreateWindow(m_Context.WIDTH, m_Context.HEIGHT, "Vulkan window", nullptr, nullptr);
	}

	glfwSetWindowUserPointer (m_Context.MainWindow, this);
	glfwSetFramebufferSizeCallback (m_Context.MainWindow, framebuffer_resize_callback);

	using namespace NutCracker;
	// Set GLFW callbacks
	glfwSetWindowSizeCallback (m_Context.MainWindow, [](GLFWwindow* window, int width, int height)
		{
			auto* app_instance = (Instancing::Instance*)(glfwGetWindowUserPointer(window));
			app_instance->m_Context.WIDTH = width;
			app_instance->m_Context.HEIGHT = height;

			WindowResizeEvent event(0, width, height);
			app_instance->onEvent (event);
		});

	glfwSetWindowCloseCallback (m_Context.MainWindow, [](GLFWwindow* window)
		{
			auto* app_instance = (Instancing::Instance*)(glfwGetWindowUserPointer(window));
			WindowCloseEvent event (0);
			app_instance->onEvent (event);
		});

	glfwSetKeyCallback (m_Context.MainWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			auto* app_instance = (Instancing::Instance*)(glfwGetWindowUserPointer(window));

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(0, key, 0);
					app_instance->onEvent (event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(0, key);
					app_instance->onEvent (event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(0, key, 1);
					app_instance->onEvent (event);
					break;
				}
			}
		});

	glfwSetCharCallback (m_Context.MainWindow, [](GLFWwindow* window, uint32_t keycode)
		{
			auto* app_instance = (Instancing::Instance*)(glfwGetWindowUserPointer(window));

			KeyTypedEvent event(0, keycode);
			app_instance->onEvent (event);
		});

	glfwSetMouseButtonCallback (m_Context.MainWindow, [](GLFWwindow* window, int button, int action, int mods)
		{
			auto* app_instance = (Instancing::Instance*)(glfwGetWindowUserPointer(window));

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(0, button);
					app_instance->onEvent (event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(0, button);
					app_instance->onEvent (event);
					break;
				}
			}
		});

	glfwSetScrollCallback (m_Context.MainWindow, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			auto* app_instance = (Instancing::Instance*)(glfwGetWindowUserPointer(window));

			MouseScrolledEvent event(0, (float)xOffset, (float)yOffset);
			app_instance->onEvent (event);
		});

	glfwSetCursorPosCallback (m_Context.MainWindow, [](GLFWwindow* window, double xPos, double yPos)
		{
			auto* app_instance = (Instancing::Instance*)(glfwGetWindowUserPointer(window));

			MouseMovedEvent event(0, (float)xPos, (float)yPos);
			app_instance->onEvent (event);
		});

	m_Camera.resize_projection (m_Context.WIDTH, m_Context.HEIGHT);

	UIElement::font_atlas = &m_Context.ShonenPunkFont;
	UIElement::s_CameraPosn = &m_Camera.Position;
	UIElement::s_CameraFocusPoint = &m_Camera.LookAt;

	m_simulationPSO.sample_start ();
}

void Instancing::Instance::update (double latency) {
//	LOG_trace("{:s} {:f}", __FUNCSIG__, latency);

	// dirty fix
	if (m_CurrentFrame == 1){
		change_ui_page(&m_UIPages[1]);
	}

	// UI button code
	if (Instancing::UIElement::s_selected != nullptr) {
		auto id = SpecialIDs(int64_t(Instancing::UIElement::s_selected->user_data));
		switch (id) {
			case SpecialIDs::worldposn_overlay_console: {
				change_ui_page(&m_UIPages[1]);
			}; break;

			case SpecialIDs::console_overlay_go_to_simulation: {
				change_ui_page(&m_UIPages[0]);
			}; break;
			case SpecialIDs::console_overlay_camera_follow_gbest: {
				m_SpecialInputGathered.console_overlay_camera_follow_gbest = !m_SpecialInputGathered.console_overlay_camera_follow_gbest;
				if (m_SpecialInputGathered.console_overlay_camera_follow_gbest)
					Instancing::UIElement::s_selected->color = {0,1,0};//{0.17f, 0.46f, 0.26f};
				else Instancing::UIElement::s_selected->color = {1,0,0};//{0.69f, 0.32f, 0.34f};
				Instancing::UIElement::s_selected = nullptr;
			}; break;
			case SpecialIDs::console_overlay_check_min_max_imize: {
				m_SpecialInputGathered.console_overlay_check_min_max_imize = !m_SpecialInputGathered.console_overlay_check_min_max_imize;
				if (m_SpecialInputGathered.console_overlay_check_min_max_imize)
					Instancing::UIElement::s_selected->color = {0.12f, 0.22f, 1}//{0.12f, 0.22f, 0.55f}
				   ,Instancing::UIElement::s_selected->text = "MINIMIZE";
				else Instancing::UIElement::s_selected->color = {0.95f, 0.86f, 0.14f}//{0.95f, 0.86f, 0.64f}
					,Instancing::UIElement::s_selected->text = "MAXIMIZE";
				m_Context.UiText.refresh = true;
				Instancing::UIElement::s_selected = nullptr;
			}; break;
			case SpecialIDs::console_overlay_input_max_iter: {
				m_SpecialInputGathered.console_overlay_input_max_iter = atoi(Instancing::UIElement::s_selected->text.c_str());
			}; break;
			case SpecialIDs::console_overlay_input_num_particles: {
				m_SpecialInputGathered.console_overlay_input_num_particles = atoi(Instancing::UIElement::s_selected->text.c_str());
			}; break;
			case SpecialIDs::console_overlay_input_workspace_x: {
				m_SpecialInputGathered.console_overlay_input_workspace_x = atof(Instancing::UIElement::s_selected->text.c_str());
			}; break;
			case SpecialIDs::console_overlay_input_workspace_y: {
				m_SpecialInputGathered.console_overlay_input_workspace_y = atof(Instancing::UIElement::s_selected->text.c_str());
			}; break;
			case SpecialIDs::console_overlay_input_workspace_z: {
				m_SpecialInputGathered.console_overlay_input_workspace_z = atof(Instancing::UIElement::s_selected->text.c_str());
			}; break;
			case SpecialIDs::console_overlay_input_inertia: {
				m_SpecialInputGathered.console_overlay_input_inertia = atof(Instancing::UIElement::s_selected->text.c_str());
			}; break;
			case SpecialIDs::console_overlay_input_cognitive_const: {
				m_SpecialInputGathered.console_overlay_input_cognitive_const = atof(Instancing::UIElement::s_selected->text.c_str());
			}; break;
			case SpecialIDs::console_overlay_input_social_const: {
				m_SpecialInputGathered.console_overlay_input_social_const = atof(Instancing::UIElement::s_selected->text.c_str());
			}; break;
			case SpecialIDs::console_overlay_try_restart_simulation: {
				std::string err;
				if (m_simulationPSO.start (err, FITNESS_FUNCTION
						,m_SpecialInputGathered.console_overlay_check_min_max_imize
						,m_SpecialInputGathered.console_overlay_input_cognitive_const
						,m_SpecialInputGathered.console_overlay_input_social_const
						,m_SpecialInputGathered.console_overlay_input_inertia
						,m_SpecialInputGathered.console_overlay_input_max_iter
						,m_SpecialInputGathered.console_overlay_input_num_particles
						,glm::vec3 (
							m_SpecialInputGathered.console_overlay_input_workspace_x,
							m_SpecialInputGathered.console_overlay_input_workspace_y,
							m_SpecialInputGathered.console_overlay_input_workspace_z
								)))

					m_Specials.console_overlay_out_display_err->text = "FOR ERROR DISPLAY", change_ui_page (&m_UIPages[0]);
				else {
					std::for_each(err.begin(), err.end(), [](char & c){ c = ::toupper(c); });
					m_Specials.console_overlay_out_display_err->text = std::move (err);
					m_Context.UiText.refresh = true;
				}
				Instancing::UIElement::s_selected = nullptr;
			}; break;
		};
	};

	if (m_Specials.worldposn_overlay_pause_play != nullptr) {
		if (Instancing::UIElement::s_selected == m_Specials.worldposn_overlay_pause_play && m_simulationPSO.max_iterations > 0) { // play animation
			uint32_t keyframe = m_CurrentFrame % m_AnimationFrameCount;
			if (keyframe == 0)
				m_simulationPSO.update ();
			for (int i = 0; auto& ui_element: current_UIPage->ui_elements) {
				if (SpecialIDs(int64_t(ui_element.user_data)) == SpecialIDs::worldposn_overlay_target) {
					ui_element.position = m_simulationPSO.particles[i].old_position + (m_simulationPSO.particles[i].position - m_simulationPSO.particles[i].old_position)*(m_AnimationChange * keyframe);
					++i;
				}
			}
			if (m_SpecialInputGathered.console_overlay_camera_follow_gbest)
				m_Camera.LookAt = glm::vec3 (m_simulationPSO.g_best), m_Camera.refresh = true;
			m_Context.ui_quads.refresh = true;
		}
	}
	if (m_Camera.refresh) {
		m_Camera.update (latency);
		if (m_Specials.worldposn_overlay_console != nullptr || m_Specials.worldposn_overlay_pause_play != nullptr) {
			auto cam_dir = m_Camera.LookAt - m_Camera.Position;
			float _dz = (cam_dir.z*cam_dir.z) / glm::dot (cam_dir, cam_dir);
			_dz = (1/std::abs (_dz - 1)) - 1;
			if (m_Specials.worldposn_overlay_console)
				m_Specials.worldposn_overlay_console->position.x = -_dz*.1 - .5;//-_dz  + 3.5f;
			if (m_Specials.worldposn_overlay_pause_play)
				m_Specials.worldposn_overlay_pause_play->position.x = +_dz*.1 + .6;//_dz - 3.4f; 
		}

		m_Camera.refresh = false;
		m_Context.ui_quads.refresh = true;
		m_Context.UiText.refresh = true;
	}

	m_Context.UiText.refresh |= m_Specials.update (current_UIPage);
	
	if (m_Context.ui_quads.refresh) { // update quads
		m_Context.ui_quads.destroy_instance_buffer (m_Context.Vk.LogicalDevice);
		m_Context.ui_quads.create_instance_buffer (m_Context.Vk.LogicalDevice, m_Context.Vk.PhysicalDevice, current_UIPage->GetQuads());
		m_Context.ui_quads.refresh = false;
	}
	if (m_Context.UiText.refresh) { // update quads
		m_Context.UiText.destroy_instance_buffer (m_Context.Vk.LogicalDevice);
		m_Context.UiText.create_instance_buffer (m_Context.Vk.LogicalDevice, m_Context.Vk.PhysicalDevice, current_UIPage);
		m_Context.UiText.refresh = false;
	}
	glfwPollEvents ();
}

void Instancing::Instance::cleanup () {
	LOG_trace(__FUNCSIG__);

	glfwDestroyWindow(m_Context.MainWindow);

    glfwTerminate();
}

void Instancing::Instance::create_ui_pages () {	
	Instancing::UIPage* page;

	constexpr int num_particles = PARTICLE_COUNT;
	constexpr int total_quads =   3 // axis
								+ 1 // show position
								+ 1 // pause/play
								+ num_particles; // equation inputs, lets say 1

	page = &m_UIPages[0];
	page->ui_elements.resize(total_quads);
	page->save_state.camera_posn  = {2.0f, -2.0f, 4.0f};
	page->save_state.camera_focus = {0, 0, 0};
	page->save_state.camera_static = false;

	int i = 0;
	// axis
	page->ui_elements[i] = Instancing::UIElement {
			.position = {0,0,0},
			.scale = {10, .1f, .1f},
			.color = {1,1,1},
			.textureID = ONLY_COLOR,
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.position = {0,0,0},
			.scale = {.1f, 10, .1f},
			.color = {1,1,1},
			.textureID = ONLY_COLOR,
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.position = {0,0,0},
			.rotation = {glm::radians(90.0f), 0, glm::radians(90.0f)},
			.scale = {10, .1f, .1f},
			.color = {1,1,1},
			.textureID = ONLY_COLOR,
		};
	++i;

	// PSO particles
	for (int j = 0; j < num_particles; ++j) {
		page->ui_elements[i] = Instancing::UIElement {
			.clickable = true,
			.looking_at_camera = true,
			.scale = {.1f, .1f, 1},
			.color = {0,1,1},
			.textureID = ONLY_COLOR,
			.user_data = (void*)int64_t(SpecialIDs::worldposn_overlay_target),
		};
		++i;
	};
	
	page->ui_elements[i] = Instancing::UIElement {
			.text = "THIS IS\nCONSOLE",
			.clickable = true,
			.looking_at_camera = true,
			.HUD = true,
			.position = {0,-.3f,0},
			.rotation = {0, 0, 0},
			.scale = {0,0,0},
			.color = {1,1,1},// {0,0,1},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::worldposn_overlay_console),
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "PAUSE/PLAY",
			.clickable = true,
			.looking_at_camera = true,
			.HUD = true,
			.position = {0,-.3f,0},
			.rotation = {0, 0, 0},
			.scale = {0,0,0},
			.color = {1,1,1},// {1,0,0},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::worldposn_overlay_pause_play),
		};
	++i;

	page->ui_elements.resize(i);

	page = &m_UIPages[1];
	page->ui_elements.resize(30);
	i = 0;
	page->save_state.camera_posn  = {0,0.000000001f, -5};
	page->save_state.camera_focus = {0, 0, 0};
	page->save_state.camera_static = true;
	page->ui_elements[i] = Instancing::UIElement {
			.position = {0,0,0},
			.scale = {6,4,0},
			.color = {1,1,1},// {1,0,0},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::worldposn_overlay_pause_play),
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "BACK",
			.clickable = true,
			.position = {2,1.5f,0},
			.scale = {1,1,0},
			.color = {1,1,1},// {0,1,0},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_go_to_simulation),
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "THIS IS CONSOLE",
			.position = {0,1.5f,0},
			.scale = {3,.8,0},
			.color = {1,1,1},// {0,0,1},
			.textureID = OTHER_TEXTURE,
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "RESTART SIM",
			.clickable = true,
			.position = {-2,-1.5f,0},
			.scale = {2,1,0},
			.color = {1,1,1},// {0,1,0},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_try_restart_simulation),
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "FOLLOW GBEST",
			.clickable = true,
			.position = {-2,1.5f,0},
			.scale = {1,1,0},
			.color = {1,1,1},// {0,1,0},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_camera_follow_gbest),
		};
	++i;
	// max iteration
	page->ui_elements[i] = Instancing::UIElement {
			.text = "MAX ITER:",
			.position = {3.1,.5f,0},
			.scale = {1,.7,0},
			.color = {1,1,1},// {1,0,0},
			.textureID = OTHER_TEXTURE,
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "",
			.clickable = true,
			.is_input = true,
			.position = {2.3,.5f,0},
			.scale = {.7,.7,0},
			.color = {1,1,1},// {1,1,0},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_input_max_iter),
		};
	++i;
	// Total particles
	page->ui_elements[i] = Instancing::UIElement {
			.text = "SPAWN PARTICLES:",
			.position = {1.35f,.5f,0},
			.scale = {1.1,.7,0},
			.color = {1,1,1},// {0,1,0},
			.textureID = OTHER_TEXTURE,
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "",
			.clickable = true,
			.is_input = true,
			.position = {0.375f,.5f,0},
			.scale = {0.9,.7,0},
			.color = {1,1,1},// {0,1,1},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_input_num_particles),
		};
	++i;
	// WORKSPACE
	page->ui_elements[i] = Instancing::UIElement {
			.text = "WORKSPACE",
			.position = {-.8,.5f,0},
			.scale = {1.2,.7,0},
			.color = {1,1,1},// {0,0,1},
			.textureID = OTHER_TEXTURE,
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "",
			.clickable = true,
			.is_input = true,
			.position = {-1.7f,.5f,0},
			.scale = {.8,.7,0},
			.color = {1,1,1},// {1,0,1},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_input_workspace_x),
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "",
			.clickable = true,
			.is_input = true,
			.position = {-2.4f,.5f,0},
			.scale = {.8,.7,0},
			.color = {1,1,1},// {1,0,0},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_input_workspace_y),
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "",
			.clickable = true,
			.is_input = true,
			.position = {-3.135,.5f,0},
			.scale = {.8,.7,0},
			.color = {1,1,1},// {0,1,0},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_input_workspace_z),
		};
	++i;
	// MIN/MAX-IMIZE
	page->ui_elements[i] = Instancing::UIElement {
			.text = "MIN/MAX-IMIZE",
			.clickable = true,
			.position = {2.7,-.5f,0},
			.scale = {1.4,.7,0},
			.color = {1,1,1},// {1,0,0},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_check_min_max_imize),
		};
	++i;
	// INERTIA
	page->ui_elements[i] = Instancing::UIElement {
			.text = "INERTIA:",
			.position = {1.6,-.5f,0},
			.scale = {.7,.7,0},
			.color = {1,1,1},// {1,1,0},
			.textureID = OTHER_TEXTURE,
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "",
			.clickable = true,
			.is_input = true,
			.position = {.85,-.5f,0},
			.scale = {.8,.7,0},
			.color = {1,1,1},// {0,1,0},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_input_inertia),
		};
	++i;
	// cognitive constant
	page->ui_elements[i] = Instancing::UIElement {
			.text = "COGNITIVE\nCONSTANT:",
			.position = {-.15,-.5f,0},
			.scale = {1,1,0},
			.color = {1,1,1},// {0,1,1},
			.textureID = OTHER_TEXTURE,
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "",
			.clickable = true,
			.is_input = true,
			.position = {-1.05,-.5f,0},
			.scale = {.8,.7,0},
			.color = {1,1,1},// {0,0,1},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_input_cognitive_const),
		};
	++i;
	// SOCIALCONSTANT
	page->ui_elements[i] = Instancing::UIElement {
			.text = "SOCIAL\nCONSTANT",
			.position = {-2.0,-.5f,0},
			.scale = {1,1,0},
			.color = {1,1,1},// {1,0,1},
			.textureID = OTHER_TEXTURE,
		};
	++i;
	page->ui_elements[i] = Instancing::UIElement {
			.text = "",
			.clickable = true,
			.is_input = true,
			.position = {-2.9,-.5f,0},
			.scale = {.8,.7,0},
			.color = {1,1,1},// {1,0,0},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_input_social_const),
		};
	++i;
	// ERR out
	page->ui_elements[i] = Instancing::UIElement {
			.text = "ERROR OUTPUT",
			.position = {1,-1.5f,0},
			.scale = {3,1,0},
			.color = {1,1,1},// {0,0,1},
			.textureID = OTHER_TEXTURE,
			.user_data = (void*)int64_t(SpecialIDs::console_overlay_out_display_err),
		};
	++i;
	page->ui_elements.resize(i);
}

void Instancing::Instance::onEvent (NutCracker::Event& e) {
	using namespace NutCracker;


	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowResizeEvent>([&](WindowResizeEvent& e) {
			m_Camera.resize_projection (e.GetWidth (), e.GetHeight ());

			return false;
		});
	dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e) {
			if (!m_Camera.is_static) {
				glm::vec3 front = glm::normalize (m_Camera.LookAt - m_Camera.Position);
				glm::vec3 right = glm::cross(front, glm::vec3(0,0,1));
				glm::vec3 up = glm::cross(right, front);
				
				if (e.GetKeyCode() == Key::Up)
					m_Camera.Position += front*.1f, m_Camera.refresh = true;
				if (e.GetKeyCode() == Key::Down)
					m_Camera.Position -= front*.1f, m_Camera.refresh = true;
				if (e.GetKeyCode() == Key::Right)
					m_Camera.Position += right*.1f, m_Camera.refresh = true;
				if (e.GetKeyCode() == Key::Left)
					m_Camera.Position -= right*.1f, m_Camera.refresh = true;
				if (e.GetKeyCode() == Key::PageUp)
					m_Camera.Position += up*.1f, m_Camera.refresh = true;
				if (e.GetKeyCode() == Key::PageDown)
					m_Camera.Position -= up*.1f, m_Camera.refresh = true;
			}
			if (e.GetKeyCode() == Key::Space)
				m_simulationPSO.sample_start ();

			static bool toggle_console = false;
			static bool toggle_iconsole = false;
			constexpr glm::vec3 console_scale = {.6f,.2f,1};
			constexpr glm::vec3 iconsole_scale = {.3f,.3f,1};
			constexpr float console_text_scale = 0.3f;
			constexpr float iconsole_text_scale = 0.3f;
			if (e.GetKeyCode() == Key::C && m_Specials.worldposn_overlay_console != nullptr) {
				m_Specials.worldposn_overlay_console->scale = console_scale*float(toggle_console), m_Specials.worldposn_overlay_console->textScale = console_text_scale*(toggle_console); 
				toggle_console = !toggle_console;

				m_Context.ui_quads.refresh = true;
				m_Context.UiText.refresh = true;
			}
			if (e.GetKeyCode() == Key::I && m_Specials.worldposn_overlay_pause_play != nullptr) {
				m_Specials.worldposn_overlay_pause_play->scale = iconsole_scale*float(toggle_iconsole), m_Specials.worldposn_overlay_pause_play->textScale = iconsole_text_scale*(toggle_iconsole); 
				toggle_iconsole = !toggle_iconsole;
				
				m_Context.ui_quads.refresh = true;
				m_Context.UiText.refresh = true;
			}
			return false;
		});
	dispatcher.Dispatch<MouseMovedEvent>([&](MouseMovedEvent& e) {
			m_Camera.mouseRecalculateRay (e, m_Context.WIDTH, m_Context.HEIGHT);
			return false;
		});
	auto [q, t] = this->current_UIPage->onEvent(e, m_Context.WIDTH, m_Context.HEIGHT, &m_Camera);
	m_Context.ui_quads.refresh |= q;
	m_Context.UiText.refresh |= t;
}

void Instancing::Instance::struct_Camera::resize_projection (int scr_width, int scr_height) {
	ProjectionMatrix = glm::perspective (glm::radians(45.0f), scr_width / float (scr_height), 0.1f, 100.0f);
}

void Instancing::Instance::struct_Camera::update (double latency) {
	ViewMatrix = glm::lookAt (Position, LookAt, glm::vec3(0.0f, 0.0f, 1.0f));
};

void Instancing::Instance::struct_Camera::mouseRecalculateRay (NutCracker::MouseMovedEvent& e, int scr_width, int scr_height) {
	float x = (2.0f * e.GetX()) / scr_width - 1.0f;
	float y = 1.0f - (2.0f * e.GetY()) / scr_height;
	float z = -1.0f;
	float w = 1.0f;
	glm::vec4 ray_clip = {x, y, z, w};
	glm::vec4 ray_eye = glm::inverse(ProjectionMatrix) * ray_clip;
	glm::vec4 ray_eye_iZ = glm::vec4 (ray_eye.x, ray_eye.y, -1.0, 0.0);
	mouse_ray_dirn = glm::vec3(glm::inverse(ViewMatrix) * ray_eye_iZ);
};
std::vector<Instancing::AnInstance> Instancing::UIElement::createTextQuads() {
	if (text.empty ())
		return {};

	double total_advance_x = 0.0;
	double advance_x = total_advance_x;
	double total_advance_y /*baseline*/ = -(font_atlas->MetricsLineHeight () + font_atlas->MetricsUnderlineY ()); // UnderlineY is negetive
	double scale_by = textScale * font_atlas->AtlasFontsize () / font_atlas->AtlasHeight ();

	std::vector<AnInstance> quads_storage; quads_storage.reserve (text.size ());
	glm::vec2 dimensions_of_font_atlas = {font_atlas->AtlasWidth (), font_atlas->AtlasHeight ()};
	glm::mat4 world_model_matrix = model_matrix;

	auto unicode_to_ascii = [](char in) -> unicode_t { return in; };
	for (char charec: text) {
		if (charec == '\n') {
			advance_x = 0.0;
			total_advance_y += font_atlas->MetricsLineHeight () + 0.2f;
			continue;
		}
		if (const auto* glyph = font_atlas->getGlyph (unicode_to_ascii (charec))) {
			if (glyph->planeBounds.right - glyph->planeBounds.left > 0.01f) {
				glm::vec2 glyph_position {advance_x + (glyph->planeBounds.right - glyph->planeBounds.left)*0.5, total_advance_y + (glyph->planeBounds.top + glyph->planeBounds.bottom)*0.5};

				glm::vec2 glyph_scale = {glyph->planeBounds.right - glyph->planeBounds.left, glyph->planeBounds.top - glyph->planeBounds.bottom};
				glm::vec2 glyph_texCoord00 = {glyph->atlasBounds.left, dimensions_of_font_atlas.y - glyph->atlasBounds.bottom};
				glm::vec2 glyph_texCoordDelta  = {glyph->atlasBounds.right - glyph->atlasBounds.left, glyph->atlasBounds.bottom - glyph->atlasBounds.top};

				quads_storage.push_back (Instancing::AnInstance {
					.model0 = {glyph_position.x, glyph_position.y, glyph_scale.x, glyph_scale.y},
					.texCoord00 = glyph_texCoord00,
					.texCoordDelta = glyph_texCoordDelta,
				});
			}
			advance_x += glyph->advance;
		} else {
			advance_x += 0.5;
		}
		total_advance_x = std::max(advance_x, total_advance_x);
	}
	glm::vec2 mid = {total_advance_x * 0.5, (total_advance_y - font_atlas->MetricsUnderlineY ()) * 0.5}; // UnderlineY is negetive
	for (auto& instance: quads_storage) {
		glm::mat4 model_matrix = world_model_matrix;
		glm::vec3 rel_posn = {(mid.x - instance.model0[0])*scale_by, (mid.y - instance.model0[1])*scale_by, 0.1f};
		glm::vec3 rel_scale = {instance.model0[2]*scale_by, instance.model0[3]*scale_by, 1};
		model_matrix = glm::translate (model_matrix, rel_posn);
		model_matrix = glm::scale (model_matrix, rel_scale);
		model_matrix = glm::transpose (model_matrix);

		instance.model0 = model_matrix[0];
		instance.model1 = model_matrix[1];
		instance.model2 = model_matrix[2];

		instance.texCoord00 /= dimensions_of_font_atlas;
		instance.texCoordDelta /= dimensions_of_font_atlas;
		instance.texID = FONT_TEXTURE;

		instance.color = {1,1,1};
	}

	return quads_storage;
}
Instancing::AnInstance Instancing::UIElement::GetQuad() {
	model_matrix = glm::mat4(1.0f);
	// inverse of opengl
	if (!looking_at_camera) {
		model_matrix = glm::translate (model_matrix, position);
		model_matrix = glm::rotate (model_matrix, rotation.y, {0,1,0});
		model_matrix = glm::rotate (model_matrix, rotation.x, {1,0,0});
		model_matrix = glm::rotate (model_matrix, rotation.z, {0,0,1});
	} else {
		glm::vec3 posn = position;
		if (HUD) {
			glm::vec3 _Z = glm::normalize(*s_CameraFocusPoint - *s_CameraPosn);
			glm::vec3 _X = glm::cross(_Z, glm::vec3(0,0,1));
			glm::vec3 _Y = glm::cross(_X,_Z);
			posn = *s_CameraPosn + _X*posn.x + _Y*posn.y + _Z*(posn.z + 1);
		}
		model_matrix = glm::inverse(glm::lookAt (posn, *s_CameraPosn, glm::vec3(0.0f, 0.0f, 1.0f)));
	}
	glm::mat4 model_matrix_sent = glm::transpose (glm::scale (model_matrix, scale));

	Instancing::AnInstance return_data {
		.model0 = model_matrix_sent[0],
		.model1 = model_matrix_sent[1],
		.model2 = model_matrix_sent[2],

		.color         = color,
		.texCoord00    = textureFrom,
		.texCoordDelta = textureFromToDiff,
		.texID = textureID,
	};

	if (s_hovered == this)
		return_data.color = s_HoverColor;
	if (s_selected == this)
		return_data.color = s_SelectColor;

	return return_data;
}
void Instancing::Instance::change_ui_page (UIPage* change_to_UIPage) {
	if (change_to_UIPage == nullptr)
		return;
	// clear
	if (m_Specials.worldposn_overlay_console != nullptr)
		m_Specials.worldposn_overlay_console->text = "THIS IS CONSOLE";

	m_Specials.worldposn_overlay_console = nullptr;
	m_Specials.worldposn_overlay_target = nullptr;
	m_Specials.worldposn_overlay_pause_play = nullptr;

	Instancing::UIElement::s_selected = nullptr;
	Instancing::UIElement::s_hovered = nullptr;

	// store 
	current_UIPage->save_state.camera_posn = m_Camera.Position;
	current_UIPage->save_state.camera_focus = m_Camera.LookAt;
	current_UIPage->save_state.camera_static = m_Camera.is_static;

	// load
	m_Camera.Position = change_to_UIPage->save_state.camera_posn;
	m_Camera.LookAt = change_to_UIPage->save_state.camera_focus;
	m_Camera.is_static = change_to_UIPage->save_state.camera_static;

	current_UIPage = change_to_UIPage;
	m_Context.ui_quads.refresh = true;
	m_Context.UiText.refresh = true;
	m_Camera.refresh = true;
}