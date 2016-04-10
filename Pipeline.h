#pragma once

#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include <string>

class Window;
class Renderer;

// Pipeline handles vulkan pipelines, it's a relatively big object so it got it's own class
// This class automatically creates a vulkan pipeline from given shader sources and window
class Pipeline
{
public:
	Pipeline( Renderer * renderer, Window * window, const std::string & name );
	~Pipeline();

	VkPipeline						GetVulkanPipeline();

	const std::string			&	GetName();

private:
	void _SubConstructor();
	void _SubDestructor();

	std::string						_name;

	Renderer					*	_renderer					= nullptr;
	Window						*	_window						= nullptr;
	VkPhysicalDevice				_gpu						= VK_NULL_HANDLE;
	VkDevice						_device						= VK_NULL_HANDLE;
	VkQueue							_queue						= VK_NULL_HANDLE;
	VkPipeline						_pipeline					= VK_NULL_HANDLE;
	VkPipelineLayout				_pipeline_layout			= VK_NULL_HANDLE;
	VkShaderModule					_shader_module_vertex		= VK_NULL_HANDLE;
	VkShaderModule					_shader_module_fragment		= VK_NULL_HANDLE;
};

