#pragma once

#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include <string>
#include <vector>

class Renderer;
class Pipeline;
class Scene;

// Window object is a child object of the Renderer and it's used to open
// individual windows where we can direct our Vulkan draw commands.
class Window
{
	friend class Renderer;

public:
	Window( Renderer * renderer, VkExtent2D dimensions, std::string window_name );
	~Window();

	void									Update();
	void									Close();

	void									Render( const std::vector<VkCommandBuffer> & command_buffers );
	void									RenderScene( const Scene * scene, bool force_recalculate = false );

	VkExtent2D								GetSize();

	const std::vector<Pipeline*>		&	GetPipelines();
	Pipeline							*	FindPipeline( std::string name );

	VkRenderPass							GetRenderPass();
	const std::vector<VkFramebuffer>	&	GetFrameBuffers();
	uint32_t								GetCurrentFrameBufferIndex();

	void									Resize( VkExtent2D size );

private:

	void _SubConstructor( VkExtent2D dimensions );
	void _SubDestructor();

	void _CreateSetupCommandPool();
	void _DestroySetupCommandPool();
	void _AllocateSetupCommandBuffer();
	void _BeginSetupCommandBuffer();
	void _EndSetupCommandBuffer();
	void _ExecuteSetupCommandBuffer();

	void _CreateOSWindow();
	void _CreateOSSurface();
	void _DestroyOSWindow();
	void _UpdateOSWindow();

	void _CreateSurface();
	void _DestroySurface();

	void _CreateSwapchain();
	void _DestroySwapchain();

	void _CreateSwapchainImages();
	void _DestroySwapchainImages();
	void _CreateDepthBuffer();
	void _DestroyDepthBuffer();

	void _CreateRenderPass();
	void _DestroyRenderPass();

	void _CreateFrameBuffers();
	void _DestroyFrameBuffers();

	void _CreateRenderCommands();
	void _DestroyRenderCommands();

	void _CreatePipelines();
	void _DestroyPipelines();

	VkDevice							_device							= VK_NULL_HANDLE;
	VkQueue								_queue							= VK_NULL_HANDLE;
	VkSwapchainKHR						_swapchain						= VK_NULL_HANDLE;
	VkSurfaceKHR						_surface						= VK_NULL_HANDLE;
	VkCommandPool						_setup_command_pool				= VK_NULL_HANDLE;
	VkCommandBuffer						_setup_command_buffer			= VK_NULL_HANDLE;
	VkImage								_depth_image					= VK_NULL_HANDLE;
	VkImageView							_depth_image_view				= VK_NULL_HANDLE;
	VkDeviceMemory						_depth_image_memory				= VK_NULL_HANDLE;
	VkRenderPass						_render_pass					= VK_NULL_HANDLE;
	VkCommandPool						_command_pool					= VK_NULL_HANDLE;
	std::vector<VkImage>				_swapchain_images;
	std::vector<VkImageView>			_swapchain_image_views;
	std::vector<VkFramebuffer>			_framebuffers;
	std::vector<VkCommandBuffer>		_render_command_buffers;
	std::vector<VkSemaphore>			_render_complete;
	VkSemaphore							_present_image_available		= VK_NULL_HANDLE;

	Renderer						*	_renderer						= VK_NULL_HANDLE;
	std::vector<Pipeline*>				_pipelines;

	VkSurfaceCapabilitiesKHR			_surface_capabilities			= {};
	VkSurfaceFormatKHR					_surface_format					= {};
	VkExtent2D							_surface_size					= { 512, 512 };
	VkFormat							_depth_format					= VK_FORMAT_UNDEFINED;
	VkFormat							_color_format					= VK_FORMAT_UNDEFINED;

	uint32_t							_swapchain_image_count			= 0;
	uint32_t							_current_swapchain_image		= 0;

	std::string							_window_name;
	bool								_window_should_close			= false;

	VkBool32							_WSI_supported					= false;

#if VK_USE_PLATFORM_WIN32_KHR
	HINSTANCE							_win32_instance					= nullptr;
	HWND								_win32_window					= nullptr;
	std::string							_win32_class_name;
	static uint64_t						_win32_class_id_counter;
#elif VK_USE_PLATFORM_XCB_KHR
	xcb_connection_t				*	_xcb_connection					= nullptr;
	xcb_screen_t					*	_xcb_screen						= nullptr;
	xcb_window_t						_xcb_window						= 0;
	xcb_intern_atom_reply_t			*	_xcb_atom_window_reply			= nullptr;
#endif
};

