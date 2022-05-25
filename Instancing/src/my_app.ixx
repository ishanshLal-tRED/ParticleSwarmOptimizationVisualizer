#include <logging.hxx>
import <random>;

export import MainApplication.Base;

import <vulkan/vulkan.h>;
import <GLFW/glfw3.h>;
export import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;

export import Helpers.FontAtlasLoader;

export module MainApplication.Instancing;
import Core.Events;

namespace Instancing {
	export constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

	export constexpr bool g_EnableValidationLayers = ( _DEBUG ? true : false );
	export constexpr bool VK_VERTEX_BUFFER_BIND_ID = 0;
	export constexpr bool VK_INSTANCE_BUFFER_BIND_ID = 1;

	// Custom structs
	export struct Vertex {
		glm::vec2 position;
		glm::vec2 texCoord;

		static constexpr VkVertexInputBindingDescription getBindingDiscription () {
			return VkVertexInputBindingDescription {
				.binding = VK_VERTEX_BUFFER_BIND_ID,
				.stride = sizeof (Vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			};
		}
		static constexpr std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		    return {
				VkVertexInputAttributeDescription {
					.location = 0,
					.binding = VK_VERTEX_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(Vertex, position),
				},
				VkVertexInputAttributeDescription {
					.location = 1,
					.binding = VK_VERTEX_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(Vertex, texCoord),
				},
			};
		}
	};

	// Custom structs
	export struct AnInstance {
		glm::vec4 model0;
		glm::vec4 model1;
		glm::vec4 model2;
		// glm::vec4 model4; // always {0,0,0,1}
		glm::vec3 color = {0.68f, 0.46f, 0.95f};
		glm::vec2 texCoord00;
		glm::vec2 texCoordDelta;
		int texID = 0;

		static constexpr VkVertexInputBindingDescription getBindingDiscription () {
			return VkVertexInputBindingDescription {
				.binding = VK_INSTANCE_BUFFER_BIND_ID,
				.stride = sizeof (AnInstance),
				.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
			};
		}
		template<uint32_t startfrom = 2>
		static constexpr std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions () {
		    return { 
				VkVertexInputAttributeDescription {
					.location = startfrom + 0,
					.binding = VK_INSTANCE_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32B32A32_SFLOAT,
					.offset = offsetof(AnInstance, model0),
				}, 
				VkVertexInputAttributeDescription {
					.location = startfrom + 1,
					.binding = VK_INSTANCE_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32B32A32_SFLOAT,
					.offset = offsetof(AnInstance, model1),
				}, 
				VkVertexInputAttributeDescription {
					.location = startfrom + 2,
					.binding = VK_INSTANCE_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32B32A32_SFLOAT,
					.offset = offsetof(AnInstance, model2),
				},
				VkVertexInputAttributeDescription {
					.location = startfrom + 3,
					.binding = VK_INSTANCE_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(AnInstance, color),
				},
				VkVertexInputAttributeDescription {
					.location = startfrom + 4,
					.binding = VK_INSTANCE_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(AnInstance, texCoord00),
				},
				VkVertexInputAttributeDescription {
					.location = startfrom + 5,
					.binding = VK_INSTANCE_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(AnInstance, texCoordDelta),
				},
				VkVertexInputAttributeDescription {
					.location = startfrom + 6,
					.binding = VK_INSTANCE_BUFFER_BIND_ID,
					.format = VK_FORMAT_R32_SINT,
					.offset = offsetof(AnInstance, texID),
				},
			};
		}
	};

	export struct UniformBufferObject {
		// meeting alignment requirements
		alignas(16) // we can do this or, #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
	    glm::mat4 projection_view;
	};

	export
	struct UIElement {
		static inline const glm::vec3 s_HoverColor = {1, 1, 0};
		static inline const glm::vec3 s_SelectColor = {0,0,1};//{0.3f, 1.0f, 0.3f};
		static Helper::FontAtlas* font_atlas;
		static UIElement* s_hovered;
		static UIElement* s_selected;
		static glm::vec3* s_CameraPosn;
		static glm::vec3* s_CameraFocusPoint;

		std::string text;

		bool clickable = false;
		bool is_input = false;
		bool looking_at_camera = false;
		bool HUD = false;

		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
		glm::vec3 color = {1,0,1};
		glm::vec2 textureFrom = {0,0}; // should be normalized
		glm::vec2 textureFromToDiff = {1,1}; // should be normalized
		float textScale = (scale.x + scale.y + scale.z)*.33f;
		int textureID; // should be normalized

		void* user_data;

		glm::mat4 model_matrix;
		AnInstance GetQuad();
		inline glm::mat4 GetFullModelMat() { return glm::scale (model_matrix, scale); }
		std::vector<AnInstance> createTextQuads ();
	
		static void Reset () {
			s_hovered  = nullptr;
			s_selected = nullptr;
		}
	};
	Helper::FontAtlas* UIElement::font_atlas = nullptr;
	UIElement* UIElement::s_hovered = nullptr;
	UIElement* UIElement::s_selected = nullptr;
	glm::vec3* UIElement::s_CameraPosn = nullptr;
	glm::vec3* UIElement::s_CameraFocusPoint = nullptr;

	export
	struct UIPage {
		struct {
			glm::vec3 camera_posn;
			glm::vec3 camera_focus;
			bool camera_static;
		} save_state;

		std::vector<UIElement> ui_elements;
		UIElement* input_at;

		std::vector<AnInstance> GetQuads () {
			std::vector<AnInstance> instance_data (std::size (ui_elements));
			for (uint32_t i = 0; i < instance_data.size (); ++i) {
				instance_data[i] = ui_elements[i].GetQuad ();
			};
			return instance_data;
		}
		std::pair<bool, bool> onEvent (NutCracker::Event& e, int scr_width, int scr_height, void* camera_ptr);
	};

	export struct Context: public Application::BaseContext {

		virtual bool KeepContextRunning() override;

		int WIDTH = 800, HEIGHT = 800;

		GLFWwindow* MainWindow = nullptr;
		
		struct struct_Vk {
			VkInstance MainInstance;
			VkDebugUtilsMessengerEXT DebugMessenger;
			VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
			VkDevice LogicalDevice;
			
			VkSurfaceKHR  Surface;

			struct {
				VkQueue Graphics;
				VkQueue Presentation;
			} Queues;

			VkSwapchainKHR Swapchain;
			std::vector<VkImage> SwapchainImages;
			std::vector<VkImageView> SwapchainImagesView;
			VkFormat SwapchainImageFormat;
			VkExtent2D SwapchainImageExtent;

			struct {
				VkShaderModule Vertex;
				VkShaderModule Fragment;
			} Modules;

			VkRenderPass RenderPass;

			VkDescriptorSetLayout DescriptorSetLayout;
			
			VkPipelineLayout PipelineLayout;
			VkPipeline GraphicsPipeline;

			std::vector<VkFramebuffer> SwapchainFramebuffers;

			VkCommandPool CommandPool;
			VkCommandBuffer CommandBuffers[MAX_FRAMES_IN_FLIGHT]; // some how array

			VkSemaphore RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
			VkSemaphore ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
			VkFence InFlightFences[MAX_FRAMES_IN_FLIGHT];

			// Thie is not the place for this struct
			struct {
				VkBuffer VertexBuffer;
				VkDeviceMemory VertexBufferMemory;
				VkBuffer IndexBuffer;
				VkDeviceMemory IndexBufferMemory;
				VkBuffer UniformBuffer [MAX_FRAMES_IN_FLIGHT];
				VkDeviceMemory UniformBufferMemory [MAX_FRAMES_IN_FLIGHT];

				VkDescriptorPool DescriptorPool;
				VkDescriptorSet DescriptorSets [MAX_FRAMES_IN_FLIGHT];

				VkImage TextureImage;
				VkDeviceMemory TextureImageMemory;
				VkImageView TextureImageView;

				VkSampler TextureSampler;
			} Extras;
		} Vk;
		
		struct struct_uitext {
			void create_instance_buffer (VkDevice logical_device, VkPhysicalDevice physical_device, UIPage* current_uipage);
			void destroy_instance_buffer (VkDevice logical_device);

			struct {
				VkBuffer InstanceBuffer = VK_NULL_HANDLE;
				VkDeviceMemory InstanceBufferMemory = VK_NULL_HANDLE;

				VkDeviceMemory FontTextureMemory;
				VkImage FontTexture;
				VkImageView FontTextureView;
				VkSampler AtlasSampler;
			} Vk;
			std::vector<AnInstance> Quads;
			bool refresh = true;
		} UiText;

		struct struct_uiquads{
			bool refresh = true;

			VkBuffer m_InstanceBuffer = VK_NULL_HANDLE;
			VkDeviceMemory m_InstanceBufferMemory = VK_NULL_HANDLE;

			uint32_t m_InstanceCount = 0;

			void create_instance_buffer (VkDevice logical_device, VkPhysicalDevice physical_device, const std::vector<Instancing::AnInstance> Quads);
			void destroy_instance_buffer (VkDevice logical_device);
		} ui_quads;

		Helper::FontAtlas ShonenPunkFont {PROJECT_ROOT_LOCATION "/assets/fonts_atlas/shonen_punk.png", PROJECT_ROOT_LOCATION "/assets/fonts_atlas/shonen_punk.json"};
	};

	export constexpr int ONLY_COLOR = 0;
	export constexpr int OTHER_TEXTURE = 1;
	export constexpr int FONT_TEXTURE = 2;

	export class Instance: public Application::BaseInstance<Context> {
	public:
		virtual void setup (const std::span<char*> &argument_list) override;
		virtual void initializeVk () override;
		virtual void update (double update_latency) override;
		virtual void render (double render_latency) override;
		virtual void terminateVk () override;
		virtual void cleanup () override;

		virtual void onEvent (NutCracker::Event& e);

		struct struct_Camera {
			bool refresh = true;
			bool is_static = false;

			glm::vec3 Position {2.0f, -2.0f, 4.0f};
			glm::vec3 LookAt {0.0f, 0.0f, 0.0f};
			glm::vec3 mouse_ray_dirn = {0,0,0}; // updated with mouse movement

			glm::mat4 ProjectionMatrix;
			glm::mat4 ViewMatrix;

			Instancing::UniformBufferObject GetUBO() {
				glm::mat4 projection = ProjectionMatrix;
				projection[1][1] *= -1;
				return {projection * ViewMatrix};
			}

			void resize_projection (int scr_width, int scr_height);

			void update (double latency);

			void mouseRecalculateRay (NutCracker::MouseMovedEvent& e, int scr_width, int scr_height);
		};
	private:
		void cleanup_swapchain_and_related ();
		void create_swapchain_and_related ();
		void recreate_swapchain_and_related ();

		void create_buffers ();

		void create_ui_pages ();
		void change_ui_page (UIPage* change_to_UIPage);

		enum SpecialIDs: int64_t{
			// from scene simulation
			worldposn_overlay_console = 1,
			worldposn_overlay_pause_play,
			worldposn_overlay_target,

			// pso console
			console_overlay_go_to_simulation,
			console_overlay_try_restart_simulation,
			console_overlay_camera_follow_gbest,
			console_overlay_input_max_iter,
			console_overlay_input_num_particles,
			console_overlay_input_workspace_x,
			console_overlay_input_workspace_y,
			console_overlay_input_workspace_z,
			console_overlay_check_min_max_imize,
			console_overlay_input_inertia,
			console_overlay_input_cognitive_const,
			console_overlay_input_social_const,
			console_overlay_out_display_err,
		};
		struct {
			UIElement* worldposn_overlay_console = nullptr;
			UIElement* worldposn_overlay_pause_play = nullptr;
			UIElement* worldposn_overlay_target = nullptr;

			UIElement* console_overlay_out_display_err = nullptr;
			bool update (UIPage* current_uipage) {
				bool refind = false;

				try {
					refind = SpecialIDs(int64_t(worldposn_overlay_console->user_data))  != SpecialIDs::worldposn_overlay_console ||
							 SpecialIDs(int64_t(worldposn_overlay_pause_play->user_data)) != SpecialIDs::worldposn_overlay_pause_play ||
							 SpecialIDs(int64_t(console_overlay_out_display_err->user_data)) != SpecialIDs::console_overlay_out_display_err;
				} catch (std::exception&) {
					refind = true;
				}

				if (refind) {
					worldposn_overlay_console = nullptr;
					console_overlay_out_display_err = nullptr;
					worldposn_overlay_pause_play = nullptr;
					for (auto& ui_element: current_uipage->ui_elements) {
						if (SpecialIDs(int64_t(ui_element.user_data)) == SpecialIDs::worldposn_overlay_console)
							worldposn_overlay_console = &ui_element;
						if (SpecialIDs(int64_t(ui_element.user_data)) == SpecialIDs::worldposn_overlay_pause_play)
							worldposn_overlay_pause_play = &ui_element;
						if (SpecialIDs(int64_t(ui_element.user_data)) == SpecialIDs::console_overlay_out_display_err)
							console_overlay_out_display_err = &ui_element;
					}
				}

				worldposn_overlay_target = nullptr;
				if (UIElement::s_hovered != nullptr && SpecialIDs(int64_t(UIElement::s_hovered->user_data)) == SpecialIDs::worldposn_overlay_target)
					worldposn_overlay_target = UIElement::s_hovered;
				
				// usage (can be anywhere)
				bool location_updated = false;
				bool overlay_updated = false;
				if (worldposn_overlay_console != nullptr && 
					worldposn_overlay_target != nullptr) {
					std::string x = fmt::format("X:{:.2f}, Y:{:.2f}, Z:{:.2f}", worldposn_overlay_target->position.x, worldposn_overlay_target->position.y, worldposn_overlay_target->position.z);
					if (worldposn_overlay_console->text != x)
						worldposn_overlay_console->text  = x, overlay_updated |= true;
				}
				return overlay_updated;
			}
		} m_Specials;
	private:
		size_t m_CurrentFrame = 0;

		UIPage m_UIPages[2];
		UIPage* current_UIPage = &m_UIPages[0]; // 0: simulation scene, 1: console

		struct_Camera m_Camera;

		struct PSOParticle { // 3 dimensional
			glm::vec3 old_position;
			glm::vec3 position;
			glm::vec3 velocity;
			glm::vec4 best; // Particles best position so far and value at that
		};
		struct PSOSimulator{
			// constexpr uint32_t dimensions = 3;
			uint32_t num_of_particles = 0;
			uint32_t max_iterations = 0;

			// Accelerations constants
			float coeff_c1; // cognitive constant (local_best)
			float coeff_c2; // social constant (global_best)
			float coeff_w; // Inertia
			glm::vec3 workspace;

			glm::vec4 g_best; // global best position and value at that
			
			float (*cost_func)(glm::vec3 input); // std::array<float, dimensions>

			std::vector<PSOParticle> particles;
			
			bool minimize_or_maximize; // false for minimize, true for maximize

			bool start (std::string& err, float (*fitness_func)(glm::vec3 input), const bool _minimize_or_maximize = false
					   ,const float cognitive_constant = .5f, const float social_constant = .5f, const float inertia = .9f
					   ,const uint32_t max_iter = 100, uint32_t total_particles = 100, glm::vec3 _workspace = {10,10,10}) {

				if (!(fitness_func != nullptr))
					err = "no fitness function";
				if (!(glm::dot(_workspace,_workspace) > 0.0f))
					err = "workspace too small";
				if (!(cognitive_constant > 0.0f))
					err = "req: cognitive_constant > 0";
				if (!(social_constant > 0.0f))
					err = "req: social_constant > 0";
				if (!(inertia > 0.0f))
					err = "req: inertia > 0";
				if (!(max_iter > 0))
					err = "req: max iterations > 0";
				if (!(total_particles > 0))
					err = "req: total particles > 0";
				if (!err.empty ())
					return false;

				cost_func = fitness_func;
				particles.resize (num_of_particles);

				coeff_c1 = cognitive_constant;
				coeff_c2 = social_constant;
				coeff_w = inertia;

				max_iterations = max_iter;
				num_of_particles = total_particles;
				minimize_or_maximize = _minimize_or_maximize;
				particles.resize (num_of_particles);
				workspace = _workspace;
			
			    std::random_device rd;  // Will be used to obtain a seed for the random number engine
			    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
			    std::uniform_real_distribution<> dis_x(-workspace[0], workspace[0]);
			    std::uniform_real_distribution<> dis_y(-workspace[1], workspace[1]);
			    std::uniform_real_distribution<> dis_z(-workspace[2], workspace[2]);

				for (auto& particle: particles) {
					particle.old_position = {0,0,0};
					particle.position = {dis_x(gen), dis_y(gen), dis_z(gen)};
					particle.velocity = {dis_x(gen), dis_y(gen), dis_z(gen)};

					particle.velocity = glm::normalize (particle.velocity);
					particle.best = {particle.position.x, particle.position.x, particle.position.x, cost_func (particle.position)};
				}

				g_best = {particles[0].position.x, particles[0].position.x, particles[0].position.x, cost_func(particles[0].position)};

				return true;
			};

			void sample_start () {
				std::string err;
				start(err, [](glm::vec3 in) -> float {
						return in.x*in.x + in.y*in.y + in.z*in.z + 11;
					});
			};

			void update (void) {
				if (max_iterations == 0)
					return;

				for (auto& particle: particles) {
				    static std::random_device rd;  // Will be used to obtain a seed for the random number engine
				    static std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
				    static std::uniform_real_distribution<> dis(0.0f, 1.0f);

					float cost = cost_func (particle.position);
					particle.old_position = particle.position;

					// update velocity
					float r1 = dis(gen), r2 = dis(gen);
					particle.velocity = coeff_w*particle.velocity 
									  + coeff_c1 * r1 * (glm::vec3(particle.best) - particle.position)
									  + coeff_c2 * r2 * (glm::vec3(g_best) - particle.position);

					// update position
					particle.position += particle.velocity;
					particle.position = glm::clamp (particle.position, -workspace, workspace);

					// update_bests
					if (minimize_or_maximize ? cost < particle.best.w : cost > particle.best.w) { // found better
						particle.best = {particle.old_position[0], particle.old_position[1], particle.old_position[2], cost};
						if (minimize_or_maximize ? cost < g_best.w : cost > g_best.w)
							g_best = {particle.old_position[0], particle.old_position[1], particle.old_position[2], cost};
					};
				}
				--max_iterations;
			};
		} m_simulationPSO;

		size_t m_AnimationFrameCount = 80;
		float m_AnimationChange = float(1.0/m_AnimationFrameCount);

		static constexpr uint32_t PARTICLE_COUNT = 100;

		struct {
			bool  console_overlay_camera_follow_gbest;
			bool  console_overlay_check_min_max_imize;
			int   console_overlay_input_max_iter;
			int   console_overlay_input_num_particles;
			float console_overlay_input_workspace_x;
			float console_overlay_input_workspace_y;
			float console_overlay_input_workspace_z;
			float console_overlay_input_inertia;
			float console_overlay_input_cognitive_const;
			float console_overlay_input_social_const;
		} m_SpecialInputGathered;
	};
}