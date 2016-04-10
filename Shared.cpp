
#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Shared.hpp"

#include <sstream>
#include <iostream>
#include <cmath>

void ErrCheck( VkResult err )
{
#if BUILD_ENABLE_REALTIME_ERROR_CHECKING
	if( err < 0 ) {
		std::ostringstream stream;
		switch( err ) {
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			stream << "VK_ERROR_OUT_OF_HOST_MEMORY";
			break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			stream << "VK_ERROR_OUT_OF_DEVICE_MEMORY";
			break;
		case VK_ERROR_INITIALIZATION_FAILED:
			stream << "VK_ERROR_INITIALIZATION_FAILED";
			break;
		case VK_ERROR_DEVICE_LOST:
			stream << "VK_ERROR_DEVICE_LOST";
			break;
		case VK_ERROR_MEMORY_MAP_FAILED:
			stream << "VK_ERROR_MEMORY_MAP_FAILED";
			break;
		case VK_ERROR_LAYER_NOT_PRESENT:
			stream << "VK_ERROR_LAYER_NOT_PRESENT";
			break;
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			stream << "VK_ERROR_EXTENSION_NOT_PRESENT";
			break;
		case VK_ERROR_FEATURE_NOT_PRESENT:
			stream << "VK_ERROR_FEATURE_NOT_PRESENT";
			break;
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			stream << "VK_ERROR_INCOMPATIBLE_DRIVER";
			break;
		case VK_ERROR_TOO_MANY_OBJECTS:
			stream << "VK_ERROR_TOO_MANY_OBJECTS";
			break;
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			stream << "VK_ERROR_FORMAT_NOT_SUPPORTED";
			break;
		case VK_ERROR_SURFACE_LOST_KHR:
			stream << "VK_ERROR_SURFACE_LOST_KHR";
			break;
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			stream << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
			break;
		case VK_SUBOPTIMAL_KHR:
			stream << "VK_SUBOPTIMAL_KHR";
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			stream << "VK_ERROR_OUT_OF_DATE_KHR";
			break;
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			stream << "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
			break;
		case VK_ERROR_VALIDATION_FAILED_EXT:
			stream << "VK_ERROR_VALIDATION_FAILED_EXT";
			break;
		default:
			stream << "VK_ERROR_unknown";
			break;
		}
		std::cout << stream.str() << std::endl;
#ifdef _WIN32
		MessageBox( nullptr, stream.str().c_str(), "Vulkan Runtime error!", MB_TOPMOST | MB_ICONERROR );
#endif
		std::exit( -1 );
	}
#endif
}

template<>
float clamp<float>( float input, float minimum, float maximum )
{
	assert( !std::isnan( minimum ) );
	assert( !std::isinf( minimum ) );
	assert( !std::isnan( maximum ) );
	assert( !std::isinf( maximum ) );
	return clamp( input, minimum, maximum );
}

template<>
double clamp<double>( double input, double minimum, double maximum )
{
	assert( !std::isnan( minimum ) );
	assert( !std::isinf( minimum ) );
	assert( !std::isnan( maximum ) );
	assert( !std::isinf( maximum ) );
	return clamp( input, minimum, maximum );
}
