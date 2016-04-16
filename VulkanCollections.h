#pragma once

#include "BUILD_OPTIONS.h"
#include "Platform.h"
#include "Shared.hpp"

struct Buffer
{
	VkBuffer					buffer;
	VkDeviceMemory				memory;
	VkMemoryRequirements		memory_requirements;
	VkMemoryPropertyFlags		memory_properties;
	uint64_t					memory_size;			// in bytes
	uint64_t					memory_alignment;		// in bytes
	uint32_t					memory_type_id;
};
