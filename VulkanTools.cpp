
#include "VulkanTools.h"

#include "BUILD_OPTIONS.h"
#include "Platform.h"
#include "Shared.hpp"

#include "Renderer.h"

void FindBufferMemoryType( Renderer * renderer, Buffer & buffer )
{
	auto &gpu_memory_properties = renderer->GetVulkanPhysicalDeviceMemoryProperties();
	VkMemoryPropertyFlags requirements_mask = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	auto memory_type_bits = buffer.memory_requirements.memoryTypeBits;
	for( uint32_t i = 0; i < gpu_memory_properties.memoryTypeCount; i++ ) {
		if( ( memory_type_bits & 1 ) == 1 ) {
			// Type is available, does it match user properties?
			if( ( gpu_memory_properties.memoryTypes[ i ].propertyFlags &
				requirements_mask ) == requirements_mask ) {
				buffer.memory_type_id = i;
				return;
			}
		}
		memory_type_bits >>= 1;
	}
	assert( 0 && "No memory type found.");
}

void AllocateBuffersMemory( Renderer * renderer, std::vector<Buffer>& buffers )
{
	auto device = renderer->GetVulkanDevice();

	for( auto &b : buffers ) {
		vkGetBufferMemoryRequirements( device, b.buffer, &b.memory_requirements );
		FindBufferMemoryType( renderer, b );

		VkMemoryAllocateInfo vertex_memory_allocate_info {};
		vertex_memory_allocate_info.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vertex_memory_allocate_info.allocationSize		= b.memory_requirements.size;
		vertex_memory_allocate_info.memoryTypeIndex		= b.memory_type_id;
		ErrCheck( vkAllocateMemory( device, &vertex_memory_allocate_info, nullptr, &b.memory ) );
	}
}

void FreeBuffersMemory( Renderer * renderer, std::vector<Buffer>& buffers )
{
	for( auto &b : buffers ) {
		vkFreeMemory( renderer->GetVulkanDevice(), b.memory, nullptr );
	}
}
