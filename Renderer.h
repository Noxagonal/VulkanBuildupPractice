#pragma once

#include "BUILD_OPTIONS.h"
#include "Shared.hpp"

#include <vector>
#include <list>
#include <string>

class Window;
class Scene;

// Render engine. Everything graphics related belongs to this class.
// This is the primary thing to include in the application.
// Destroying Renderer object is enough to shutdown the whole rendering
// engine and free all resources used by this object or it's child objects.
class Renderer
{
	friend class Window;

public:
	Renderer( const std::vector<std::string> & used_pipeline_names );
	~Renderer();

	Window								*	OpenWindow( VkExtent2D dimensions, std::string window_name = std::string() );

	Scene								*	CreateScene();

	bool									Run();

	const std::vector<std::string>		&	GetPipelineNames();

	const std::list<Scene*>				*	GetSceneList();
	const std::list<Window*>			*	GetWindowList();

	VkPhysicalDevice						GetVulkanPhysicalDevice();
	VkPhysicalDeviceMemoryProperties		GetVulkanPhysicalDeviceMemoryProperties();
	VkQueue									GetVulkanQueue();
	VkDevice								GetVulkanDevice();
	uint32_t								GetVulkanGraphicsQueueFamilyIndex();

private:
	void _DestroyScenes();
	void _DestroyWindows();

	void _SetupLayersAndExtensions();

	void _CreateInstance();
	void _DestroyInstance();

	void _CreateDevice();
	void _DestroyDevice();

	void _SetupDebug();
	void _CreateDebug();
	void _DestroyDebug();

	std::list<Window*>						_windows;
	std::list<Scene*>						_scenes;

	VkInstance								_instance						= VK_NULL_HANDLE;
	VkPhysicalDevice						_gpu							= VK_NULL_HANDLE;
	VkDevice								_device							= VK_NULL_HANDLE;
	VkQueue									_queue							= VK_NULL_HANDLE;

	VkPhysicalDeviceMemoryProperties		_gpu_memory_properties			= {};
	uint32_t								_render_queue_family_index		= 0;

	std::vector<const char*>				_instance_layers;
	std::vector<const char*>				_instance_extensions;
	std::vector<const char*>				_device_layers;
	std::vector<const char*>				_device_extensions;

	std::vector<std::string>				_pipeline_names;

	VkDebugReportCallbackEXT				_debug_report								= VK_NULL_HANDLE;
	VkDebugReportCallbackCreateInfoEXT	*	_debug_report_callback_create_info			= nullptr;
};
