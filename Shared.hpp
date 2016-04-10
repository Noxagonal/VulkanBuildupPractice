#pragma once

#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include <assert.h>

// Runtime error checking on results of Vulkan functions.
void ErrCheck( VkResult err );

// basic clamping function
template<typename T>
T clamp( T input, T minimum, T maximum )
{
	assert( minimum <= maximum );
	if( input < minimum ) input = minimum;
	if( input > maximum ) input = maximum;
	return input;
}
