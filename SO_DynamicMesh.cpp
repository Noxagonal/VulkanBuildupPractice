
#include "BUILD_OPTIONS.h"
#include "Platform.h"
#include "VulkanTools.h"

#include "SO_DynamicMesh.h"
#include "Shared.hpp"
#include "Renderer.h"
#include "Pipeline.h"
#include "Window.h"
#include "Mesh.h"

#include <assert.h>
#include <cstring>

SO_DynamicMesh::SO_DynamicMesh( Scene * parent_scene, Renderer * renderer, Mesh * mesh )
	: SceneObject( parent_scene, renderer )
{
	assert( nullptr != mesh );
	_mesh = mesh;
	_Initialize();
}


SO_DynamicMesh::~SO_DynamicMesh()
{
	FreeBuffersMemory( _renderer, _buffers );
	vkDestroyBuffer( _device, _buffers[ 0 ].buffer, nullptr );
	vkDestroyBuffer( _device, _buffers[ 1 ].buffer, nullptr );
}

void SO_DynamicMesh::Update()
{
	void *data = nullptr;
	vkMapMemory( _device, _buffers[ 0 ].memory, 0, _buffers[ 0 ].memory_size, 0, &data );
	memcpy( data, _local_vertices.data(), _buffers[ 0 ].memory_size );
	vkUnmapMemory( _device, _buffers[ 0 ].memory );
}

const std::vector<Mesh_Vertex> & SO_DynamicMesh::GetVertices() const
{
	return _local_vertices;
}

std::vector<Mesh_Vertex> & SO_DynamicMesh::GetEditableVertices()
{
	return _local_vertices;
}

const std::vector<Mesh_Polygon>& SO_DynamicMesh::GetIndeces() const
{
	return _local_indices;
}

std::vector<Mesh_Polygon>& SO_DynamicMesh::GetEditableIndices()
{
	return _local_indices;
}


void SO_DynamicMesh::_Initialize()
{
	// copy date over to object local space
	_local_vertices					= *_mesh->GetVerticesList();
	_local_indices					= *_mesh->GetIndicesList();

	_buffers.resize( 2 );
	_buffers[ 0 ].memory_size		= _mesh->GetVerticesListByteSize();
	_buffers[ 1 ].memory_size		= _mesh->GetIndicesListByteSize();
	_buffers[ 0 ].memory_properties	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	_buffers[ 1 ].memory_properties	= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	// create buffers
	VkBufferCreateInfo vertex_buffer_create_info {};
	vertex_buffer_create_info.sType						= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertex_buffer_create_info.size						= _buffers[ 0 ].memory_size;
	vertex_buffer_create_info.usage						= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertex_buffer_create_info.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	ErrCheck( vkCreateBuffer( _device, &vertex_buffer_create_info, nullptr, &_buffers[ 0 ].buffer ) );

	VkBufferCreateInfo index_buffer_create_info {};
	index_buffer_create_info.sType						= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	index_buffer_create_info.size						= _buffers[ 1 ].memory_size;
	index_buffer_create_info.usage						= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	index_buffer_create_info.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	ErrCheck( vkCreateBuffer( _device, &index_buffer_create_info, nullptr, &_buffers[ 1 ].buffer ) );

	AllocateBuffersMemory( _renderer, _buffers );

	{
		void *data = nullptr;
		ErrCheck( vkMapMemory( _device, _buffers[ 0 ].memory, 0, _buffers[ 0 ].memory_size, 0, &data ) );
		memcpy( data, _local_vertices.data(), _local_vertices.size() * sizeof( Mesh_Vertex ) );
		vkUnmapMemory( _device, _buffers[ 0 ].memory );
	}
	{
		void *data = nullptr;
		ErrCheck( vkMapMemory( _device, _buffers[ 1 ].memory, 0, _buffers[ 1 ].memory_size, 0, &data ) );
		memcpy( data, _local_indices.data(), _local_indices.size() * sizeof( Mesh_Polygon ) );
		vkUnmapMemory( _device, _buffers[ 1 ].memory );
	}
	ErrCheck( vkBindBufferMemory( _device, _buffers[ 0 ].buffer, _buffers[ 0 ].memory, 0 ) );
	ErrCheck( vkBindBufferMemory( _device, _buffers[ 1 ].buffer, _buffers[ 1 ].memory, 0 ) );
}


void SO_DynamicMesh::_RebuildCommandBuffer()
{
	if( nullptr == _window || nullptr == _pipeline ) {
		return;
	}

	// free old command buffers if they exist, we rescale this list to match our framebuffer image count
	if( _command_buffers.size() ) {
		vkFreeCommandBuffers( _device, _command_pool, _command_buffers.size(), _command_buffers.data() );
	}
	auto new_buffer_count				= _window->GetFrameBuffers().size();
	_command_buffers.resize( new_buffer_count );

	// allocate new command buffers exactly the amount of our window framebuffer image count
	VkCommandBufferAllocateInfo	allocate_info {};
	allocate_info.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool			= _command_pool;
	allocate_info.commandBufferCount	= new_buffer_count;
	allocate_info.level					= VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	ErrCheck( vkAllocateCommandBuffers( _device, &allocate_info, _command_buffers.data() ) );

	// for each buffer, we record the whole thing
	for( uint32_t i=0; i < new_buffer_count; ++i ) {

		VkCommandBufferInheritanceInfo inheritance_info {};
		inheritance_info.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritance_info.renderPass				= _window->GetRenderPass();
		inheritance_info.subpass				= 0;
		inheritance_info.framebuffer			= _window->GetFrameBuffers()[ i ];

		VkCommandBufferBeginInfo begin_info {};
		begin_info.sType						= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags						= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		begin_info.pInheritanceInfo				= &inheritance_info;
		vkBeginCommandBuffer( _command_buffers[ i ] , &begin_info );
		/*
		// I'm not sure if these barriers are actually needed
		// Barrier for vertex buffer transfer
		VkBufferMemoryBarrier vertex_buffer_barrier {};
		vertex_buffer_barrier.sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		vertex_buffer_barrier.srcAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
		vertex_buffer_barrier.dstAccessMask			= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		vertex_buffer_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		vertex_buffer_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		vertex_buffer_barrier.buffer				= _buffers[ 0 ].buffer;
		vertex_buffer_barrier.offset				= 0;
		vertex_buffer_barrier.size					= _buffers[ 0 ].memory_size;

		vkCmdPipelineBarrier( _command_buffers[ i ],
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			0,
			0, nullptr,
			1, &vertex_buffer_barrier,
			0, nullptr );

		// Barrier for index buffer transfer
		VkBufferMemoryBarrier index_buffer_barrier {};
		index_buffer_barrier.sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		index_buffer_barrier.srcAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
		index_buffer_barrier.dstAccessMask			= VK_ACCESS_INDEX_READ_BIT;
		index_buffer_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		index_buffer_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		index_buffer_barrier.buffer					= _buffers[ 1 ].buffer;
		index_buffer_barrier.offset					= 0;
		index_buffer_barrier.size					= _buffers[ 1 ].memory_size;

		vkCmdPipelineBarrier( _command_buffers[ i ],
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			0,
			0, nullptr,
			1, &index_buffer_barrier,
			0, nullptr );
			*/
		vkCmdBindPipeline( _command_buffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->GetVulkanPipeline() );

		VkDeviceSize vertex_buffer_offsets[] { 0 };
		vkCmdBindVertexBuffers( _command_buffers[ i ], 0, 1, &_buffers[ 0 ].buffer, vertex_buffer_offsets );
		vkCmdBindIndexBuffer( _command_buffers[ i ], _buffers[ 1 ].buffer, 0, VK_INDEX_TYPE_UINT32 );

//		vkCmdDraw( _command_buffer, 3, 1, 0, 0 );
		vkCmdDrawIndexed( _command_buffers[ i ], 3 * _local_indices.size(), 1, 0, 0, 0 );

		vkEndCommandBuffer( _command_buffers[ i ] );
	}
}
