
#include <vector>

#include "SceneObject.h"
#include "Window.h"
#include "Renderer.h"


SceneObject::SceneObject( Renderer *renderer )
{
	_renderer						= renderer;
	_device							= renderer->GetVulkanDevice();
	_queue							= renderer->GetVulkanQueue();
	_graphics_queue_family_index	= renderer->GetVulkanGraphicsQueueFamilyIndex();

	VkCommandPoolCreateInfo create_info {};
	create_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.queueFamilyIndex	= _graphics_queue_family_index;
	create_info.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkCreateCommandPool( _device, &create_info, nullptr, &_command_pool );
}

SceneObject::~SceneObject()
{
	vkDestroyCommandPool( _device, _command_pool, nullptr );
}


VkCommandBuffer SceneObject::GetActiveCommandBuffer( bool rebuild_buffers )
{
	if( _command_buffer_out_of_date || rebuild_buffers ) {
		_RebuildCommandBuffer();
		_command_buffer_out_of_date = false;
	}
	return _command_buffers[ _window->GetCurrentFrameBufferIndex() ];
}

void SceneObject::SetActiveWindow( Window * window )
{
	if( _window != window ) {
		_command_buffer_out_of_date = true;
	}
	_window = window;
}

void SceneObject::SetActivePipeline( Pipeline * pipeline )
{
	if( _pipeline != pipeline ) {
		_command_buffer_out_of_date = true;
	}
	_pipeline = pipeline;
}
