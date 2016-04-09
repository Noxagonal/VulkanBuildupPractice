#pragma once

#include <vulkan/vulkan.h>
#include <string>

class Window;
class Renderer;

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

