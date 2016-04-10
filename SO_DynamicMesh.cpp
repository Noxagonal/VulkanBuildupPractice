
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
	vkFreeMemory( _device, _vertex_buffer_memory, nullptr );
	vkFreeMemory( _device, _index_buffer_memory, nullptr );
	vkDestroyBuffer( _device, _vertex_buffer, nullptr );
	vkDestroyBuffer( _device, _index_buffer, nullptr );
}

void SO_DynamicMesh::Update()
{
	void *data = nullptr;
	vkMapMemory( _device, _vertex_buffer_memory, 0, _vertex_buffer_size, 0, &data );
	memcpy( data, _local_vertices.data(), _vertex_buffer_size );
	vkUnmapMemory( _device, _vertex_buffer_memory );
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
	_local_vertices			= *_mesh->GetVerticesList();
	_local_indices			= *_mesh->GetIndicesList();

	_vertex_buffer_size		= _mesh->GetVerticesListByteSize();
	_index_buffer_size		= _mesh->GetIndicesListByteSize();

	// create buffers
	VkBufferCreateInfo vertex_buffer_create_info {};
	vertex_buffer_create_info.sType						= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertex_buffer_create_info.size						= _vertex_buffer_size;
	vertex_buffer_create_info.usage						= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertex_buffer_create_info.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	ErrCheck( vkCreateBuffer( _device, &vertex_buffer_create_info, nullptr, &_vertex_buffer ) );

	VkBufferCreateInfo index_buffer_create_info {};
	index_buffer_create_info.sType						= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	index_buffer_create_info.size						= _index_buffer_size;
	index_buffer_create_info.usage						= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	index_buffer_create_info.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	ErrCheck( vkCreateBuffer( _device, &index_buffer_create_info, nullptr, &_index_buffer ) );

	VkMemoryRequirements vertex_buffer_requirements;
	VkMemoryRequirements index_buffer_requirements;
	vkGetBufferMemoryRequirements( _device, _vertex_buffer, &vertex_buffer_requirements );
	vkGetBufferMemoryRequirements( _device, _index_buffer, &index_buffer_requirements );
	uint32_t vertex_memory_id		= UINT32_MAX;
	uint32_t index_memory_id		= UINT32_MAX;

	auto gpu_memory_properties = _renderer->GetVulkanPhysicalDeviceMemoryProperties();
	{
		VkMemoryPropertyFlags requirements_mask = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		auto memory_type_bits = vertex_buffer_requirements.memoryTypeBits;
		for( uint32_t i = 0; i < gpu_memory_properties.memoryTypeCount; i++ ) {
			if( ( memory_type_bits & 1 ) == 1 ) {
				// Type is available, does it match user properties?
				if( ( gpu_memory_properties.memoryTypes[ i ].propertyFlags &
					requirements_mask ) == requirements_mask ) {
					vertex_memory_id = i;
				}
			}
			memory_type_bits >>= 1;
		}
		assert( vertex_memory_id != UINT32_MAX );
	}
	{
		VkMemoryPropertyFlags requirements_mask = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		auto memory_type_bits = index_buffer_requirements.memoryTypeBits;
		for( uint32_t i = 0; i < gpu_memory_properties.memoryTypeCount; i++ ) {
			if( ( memory_type_bits & 1 ) == 1 ) {
				// Type is available, does it match user properties?
				if( ( gpu_memory_properties.memoryTypes[ i ].propertyFlags &
					requirements_mask ) == requirements_mask ) {
					index_memory_id = i;
				}
			}
			memory_type_bits >>= 1;
		}
		assert( index_memory_id != UINT32_MAX );
	}

	VkMemoryAllocateInfo vertex_memory_allocate_info {};
	vertex_memory_allocate_info.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vertex_memory_allocate_info.allocationSize		= vertex_buffer_requirements.size;
	vertex_memory_allocate_info.memoryTypeIndex		= vertex_memory_id;
	ErrCheck( vkAllocateMemory( _device, &vertex_memory_allocate_info, nullptr, &_vertex_buffer_memory ) );

	VkMemoryAllocateInfo index_memory_allocate_info {};
	index_memory_allocate_info.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	index_memory_allocate_info.allocationSize		= index_buffer_requirements.size;
	index_memory_allocate_info.memoryTypeIndex		= index_memory_id;
	ErrCheck( vkAllocateMemory( _device, &index_memory_allocate_info, nullptr, &_index_buffer_memory ) );
	{
		void *data = nullptr;
		ErrCheck( vkMapMemory( _device, _vertex_buffer_memory, 0, vertex_buffer_requirements.size, 0, &data ) );
		memcpy( data, _local_vertices.data(), _local_vertices.size() * sizeof( Mesh_Vertex ) );
		vkUnmapMemory( _device, _vertex_buffer_memory );
	}
	{
		void *data = nullptr;
		ErrCheck( vkMapMemory( _device, _index_buffer_memory, 0, index_buffer_requirements.size, 0, &data ) );
		memcpy( data, _local_indices.data(), _local_indices.size() * sizeof( Mesh_Polygon ) );
		vkUnmapMemory( _device, _index_buffer_memory );
	}
	ErrCheck( vkBindBufferMemory( _device, _vertex_buffer, _vertex_buffer_memory, 0 ) );
	ErrCheck( vkBindBufferMemory( _device, _index_buffer, _index_buffer_memory, 0 ) );
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

		VkBufferMemoryBarrier vertex_buffer_barrier {};
		vertex_buffer_barrier.sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		vertex_buffer_barrier.srcAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
		vertex_buffer_barrier.dstAccessMask			= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		vertex_buffer_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		vertex_buffer_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		vertex_buffer_barrier.buffer				= _vertex_buffer;
		vertex_buffer_barrier.offset				= 0;
		vertex_buffer_barrier.size					= _vertex_buffer_size;

		vkCmdPipelineBarrier( _command_buffers[ i ],
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			0,
			0, nullptr,
			1, &vertex_buffer_barrier,
			0, nullptr );

		VkBufferMemoryBarrier index_buffer_barrier {};
		index_buffer_barrier.sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		index_buffer_barrier.srcAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
		index_buffer_barrier.dstAccessMask			= VK_ACCESS_INDEX_READ_BIT;
		index_buffer_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		index_buffer_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		index_buffer_barrier.buffer					= _index_buffer;
		index_buffer_barrier.offset					= 0;
		index_buffer_barrier.size					= _index_buffer_size;

		vkCmdPipelineBarrier( _command_buffers[ i ],
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			0,
			0, nullptr,
			1, &index_buffer_barrier,
			0, nullptr );

		vkCmdBindPipeline( _command_buffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->GetVulkanPipeline() );

		VkDeviceSize vertex_buffer_offsets[] { 0 };
		vkCmdBindVertexBuffers( _command_buffers[ i ], 0, 1, &_vertex_buffer, vertex_buffer_offsets );
		vkCmdBindIndexBuffer( _command_buffers[ i ], _index_buffer, 0, VK_INDEX_TYPE_UINT32 );

//		vkCmdDraw( _command_buffer, 3, 1, 0, 0 );
		vkCmdDrawIndexed( _command_buffers[ i ], 3 * _local_indices.size(), 1, 0, 0, 0 );

		vkEndCommandBuffer( _command_buffers[ i ] );
	}

	/*
	VkCommandBufferInheritanceInfo inheritance_info {};
	inheritance_info.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritance_info.renderPass				= _window->GetRenderPass();
	inheritance_info.subpass				= 0;
	inheritance_info.framebuffer			= 0; _window->GetFrameBuffers()[ _window->GetCurrentFrameBufferIndex() ];
//	inheritance_info.occlusionQueryEnable	= VK_TRUE;
//	inheritance_info.queryFlags				= VK_QUERY_CONTROL_PRECISE_BIT;
//	inheritance_info.pipelineStatistics		=;

	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType						= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags						= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	begin_info.pInheritanceInfo				= &inheritance_info;
	vkBeginCommandBuffer( _command_buffer, &begin_info );

	VkBufferMemoryBarrier vertex_buffer_barrier {};
	vertex_buffer_barrier.sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	vertex_buffer_barrier.srcAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
	vertex_buffer_barrier.dstAccessMask			= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	vertex_buffer_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	vertex_buffer_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	vertex_buffer_barrier.buffer				= _vertex_buffer;
	vertex_buffer_barrier.offset				= 0;
	vertex_buffer_barrier.size					= _vertex_buffer_size;

	vkCmdPipelineBarrier( _command_buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		0,
		0, nullptr,
		1, &vertex_buffer_barrier,
		0, nullptr );

	VkBufferMemoryBarrier index_buffer_barrier {};
	index_buffer_barrier.sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	index_buffer_barrier.srcAccessMask			= VK_ACCESS_HOST_WRITE_BIT;
	index_buffer_barrier.dstAccessMask			= VK_ACCESS_INDEX_READ_BIT;
	index_buffer_barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	index_buffer_barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	index_buffer_barrier.buffer					= _index_buffer;
	index_buffer_barrier.offset					= 0;
	index_buffer_barrier.size					= _index_buffer_size;

	vkCmdPipelineBarrier( _command_buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		0,
		0, nullptr,
		1, &index_buffer_barrier,
		0, nullptr );

	vkCmdBindPipeline( _command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->GetVulkanPipeline() );

	VkDeviceSize vertex_buffer_offsets[] { 0 };
	vkCmdBindVertexBuffers( _command_buffer, 0, 1, &_vertex_buffer, vertex_buffer_offsets );
	vkCmdBindIndexBuffer( _command_buffer, _index_buffer, 0, VK_INDEX_TYPE_UINT32 );

//	vkCmdDraw( _command_buffer, 3, 1, 0, 0 );
	vkCmdDrawIndexed( _command_buffer, 3 * _local_indices.size(), 1, 0, 0, 0 );

	vkEndCommandBuffer( _command_buffer );
	*/
}
