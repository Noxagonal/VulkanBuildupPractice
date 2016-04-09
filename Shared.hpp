#pragma once

#include <assert.h>
#include <vulkan/vulkan.h>

void ErrCheck( VkResult err );

template<typename T>
T clamp( T input, T minimum, T maximum )
{
	assert( minimum <= maximum );
	if( input < minimum ) input = minimum;
	if( input > maximum ) input = maximum;
	return input;
}
