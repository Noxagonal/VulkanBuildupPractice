
#include "BUILD_OPTIONS.h"
#include "Platform.h"
#include "Renderer.h"
#include "Window.h"
#include "Scene.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <assert.h>

Renderer::Renderer( const std::vector<std::string> & used_pipeline_names )
{
	_pipeline_names			= used_pipeline_names;

	_SetupLayersAndExtensions();
	_SetupDebug();
	_CreateInstance();
	_CreateDebug();
	_CreateDevice();
}


Renderer::~Renderer()
{
	_DestroyScenes();
	_DestroyWindows();
	_DestroyDevice();
	_DestroyDebug();
	_DestroyInstance();
}


Window * Renderer::OpenWindow( VkExtent2D dimensions, std::string window_name )
{
	Window		*	w	= new Window( this, dimensions , window_name );
	_windows.push_back( w );
	return w;
}


Scene * Renderer::CreateScene()
{
	Scene * scene = new Scene( this );
	_scenes.push_back( scene );
	return scene;
}

bool Renderer::Run()
{
	std::vector<Window*> windows_to_destroy;
	for( auto w : _windows ) {
		w->Update();
		if( w->_window_should_close ) {
			windows_to_destroy.push_back( w );
		}
	}
	for( auto w : windows_to_destroy ) {
		delete w;
		_windows.remove( w );
	}
	if( _windows.size() == 0 ) {
		return false;
	}
	return true;
}

const std::vector<std::string> & Renderer::GetPipelineNames()
{
	return _pipeline_names;
}

const std::list<Scene*> * Renderer::GetSceneList()
{
	return &_scenes;
}

const std::list<Window*> * Renderer::GetWindowList()
{
	return &_windows;
}

VkPhysicalDevice Renderer::GetVulkanPhysicalDevice()
{
	return _gpu;
}

VkPhysicalDeviceMemoryProperties Renderer::GetVulkanPhysicalDeviceMemoryProperties()
{
	return _gpu_memory_properties;
}

VkQueue Renderer::GetVulkanQueue()
{
	return _queue;
}

VkDevice Renderer::GetVulkanDevice()
{
	return _device;
}

uint32_t Renderer::GetVulkanGraphicsQueueFamilyIndex()
{
	return _render_queue_family_index;
}


void Renderer::_DestroyScenes()
{
	for( auto scene : _scenes ) {
		delete scene;
	}
	_scenes.clear();
}

void Renderer::_DestroyWindows()
{
	for( auto window : _windows ) {
		delete window;
	}
	_windows.clear();
}


void Renderer::_SetupLayersAndExtensions()
{
//	_instance_extensions.push_back( VK_KHR_DISPLAY_EXTENSION_NAME );			// render to screen directly, embedded systems might use this
	_instance_extensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );			// render to screen via operating system
	_instance_extensions.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );

	_device_extensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
}


void Renderer::_CreateInstance()
{
	VkApplicationInfo application_info {};
	application_info.sType					= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.apiVersion				= VK_MAKE_VERSION( 1, 0, 2 );			// 1.0.2 should be available on all vulkan enabled drivers

	VkInstanceCreateInfo create_info {};
	create_info.sType						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo			= &application_info;
	create_info.enabledLayerCount			= _instance_layers.size();
	create_info.ppEnabledLayerNames			= _instance_layers.data();
	create_info.enabledExtensionCount		= _instance_extensions.size();
	create_info.ppEnabledExtensionNames		= _instance_extensions.data();
	create_info.pNext						= _debug_report_callback_create_info;

	ErrCheck( vkCreateInstance( &create_info, nullptr, &_instance ) );
}


void Renderer::_DestroyInstance()
{
	vkDestroyInstance( _instance, nullptr );
	_instance = nullptr;
}


void Renderer::_CreateDevice()
{
	{
		uint32_t gpu_count = 0;
		vkEnumeratePhysicalDevices( _instance, &gpu_count, nullptr );
		std::vector<VkPhysicalDevice> gpu_list( gpu_count );
		vkEnumeratePhysicalDevices( _instance, &gpu_count, gpu_list.data() );
		_gpu = gpu_list[ 0 ];
	}
	{
		uint32_t family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( _gpu, &family_count, nullptr );
		std::vector<VkQueueFamilyProperties> family_list( family_count );
		vkGetPhysicalDeviceQueueFamilyProperties( _gpu, &family_count, family_list.data() );
		bool found = false;
		for( uint32_t i=0; i < family_count; ++i ) {
			if( family_list[ i ].queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
				found = true;
				_render_queue_family_index = i;
				break;
			}
		}
	}
	float queue_priorities[] { 1.0f };
	VkDeviceQueueCreateInfo queue_create_info {};
	queue_create_info.sType					= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.pQueuePriorities		= queue_priorities;
	queue_create_info.queueCount			= 1;
	queue_create_info.queueFamilyIndex		= _render_queue_family_index;

	VkPhysicalDeviceFeatures features {};
	features.shaderClipDistance				= VK_TRUE;

	VkDeviceCreateInfo create_info {};
	create_info.sType						= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount		= 1;
	create_info.pQueueCreateInfos			= &queue_create_info;
	create_info.enabledLayerCount			= _device_layers.size();
	create_info.ppEnabledLayerNames			= _device_layers.data();
	create_info.enabledExtensionCount		= _device_extensions.size();
	create_info.ppEnabledExtensionNames		= _device_extensions.data();
	create_info.pEnabledFeatures			= &features;

	ErrCheck( vkCreateDevice( _gpu, &create_info, nullptr, &_device ) );

	vkGetDeviceQueue( _device, _render_queue_family_index, 0, &_queue );
	vkGetPhysicalDeviceMemoryProperties( _gpu, &_gpu_memory_properties );
}


void Renderer::_DestroyDevice()
{
	vkDestroyDevice( _device, nullptr );
	_device = nullptr;
}


#if BUILD_ENABLE_VULKAN_ERROR_REPORTING
VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(
	VkDebugReportFlagsEXT		flags,
	VkDebugReportObjectTypeEXT	object,
	uint64_t					object_src,
	size_t						location,
	int32_t						code,
	const char *				layer_prefix,
	const char *				msg,
	void *						user_data
	)
{
	std::ostringstream stream;
	stream << "Vulkan Debug: ";
	if( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
		stream << "INFO | ";
	}
	if( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
		stream << "WARNING | ";
	}
	if( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) {
		stream << "PERFORMANCE WARNING | ";
	}
	if( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
		stream << "ERROR | ";
	}
	if( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
		stream << "DEBUG | ";
	}
	stream << "@[" << layer_prefix << "] | ";
	stream << msg << std::endl;
	std::cout << stream.str();

	if( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
#ifdef _WIN32
		MessageBox( nullptr, stream.str().c_str(), "VULKAN ERROR!", MB_TOPMOST | MB_ICONERROR );
#endif
		assert( 0 && "VULKAN_ERROR!" );
		std::exit( -1 );
	}

	return false;
}
#endif


void Renderer::_SetupDebug()
{
#if BUILD_ENABLE_VULKAN_ERROR_REPORTING
	_debug_report_callback_create_info					= new VkDebugReportCallbackCreateInfoEXT;
	_debug_report_callback_create_info->sType			= VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	_debug_report_callback_create_info->pNext			= nullptr;
	_debug_report_callback_create_info->pUserData		= nullptr;
	_debug_report_callback_create_info->pfnCallback		= VulkanDebugCallback;
	_debug_report_callback_create_info->flags			=
//		VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
//		VK_DEBUG_REPORT_DEBUG_BIT_EXT |
		0;

	_instance_layers.push_back( "VK_LAYER_LUNARG_standard_validation" );

	_instance_extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );

	_device_layers.push_back( "VK_LAYER_LUNARG_standard_validation" );
#endif
}


PFN_vkCreateDebugReportCallbackEXT		fvkCreateDebugReportCallbackEXT		= nullptr;
PFN_vkDestroyDebugReportCallbackEXT		fvkDestroyDebugReportCallbackEXT		= nullptr;


void Renderer::_CreateDebug()
{
#if BUILD_ENABLE_VULKAN_ERROR_REPORTING
	fvkCreateDebugReportCallbackEXT		= (PFN_vkCreateDebugReportCallbackEXT)		vkGetInstanceProcAddr( _instance, "vkCreateDebugReportCallbackEXT" );
	fvkDestroyDebugReportCallbackEXT	= (PFN_vkDestroyDebugReportCallbackEXT)		vkGetInstanceProcAddr( _instance, "vkDestroyDebugReportCallbackEXT" );
	if( nullptr == fvkCreateDebugReportCallbackEXT || nullptr == fvkDestroyDebugReportCallbackEXT ) {
		std::exit( -1 );
	}

	ErrCheck( fvkCreateDebugReportCallbackEXT( _instance, _debug_report_callback_create_info, nullptr, &_debug_report ) );
#endif
}


void Renderer::_DestroyDebug()
{
#if BUILD_ENABLE_VULKAN_ERROR_REPORTING
	fvkDestroyDebugReportCallbackEXT( _instance, _debug_report, nullptr );
	delete _debug_report_callback_create_info;
	_debug_report							= VK_NULL_HANDLE;
	_debug_report_callback_create_info		= nullptr;
#endif
}
