#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class Scene;
class Renderer;
class Window;
class Pipeline;

class SceneObject
{
public:
	SceneObject( Scene * parent_scene, Renderer * renderer );
	virtual ~SceneObject();

	virtual void					Update() = 0;

	VkCommandBuffer					GetActiveCommandBuffer( bool rebuild_buffers = false );
	void							SetActiveWindow( Window * window );
	void							SetActivePipeline( Pipeline * pipeline );

protected:
	Scene						*	_parent							= nullptr;
	Renderer					*	_renderer						= nullptr;
	Window						*	_window							= nullptr;
	Pipeline					*	_pipeline						= nullptr;
	VkDevice						_device							= VK_NULL_HANDLE;
	VkQueue							_queue							= VK_NULL_HANDLE;

	uint32_t						_graphics_queue_family_index	= 0;

	VkCommandPool					_command_pool					= VK_NULL_HANDLE;
	std::vector<VkCommandBuffer>	_command_buffers;

	bool							_command_buffer_out_of_date		= true;

	virtual void					_Initialize()					= 0;
	virtual void					_RebuildCommandBuffer()			= 0;
};
