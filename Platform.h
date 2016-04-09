#pragma once

#include "BUILD_OPTIONS.h"

#ifdef _WIN32

#define VK_USE_PLATFORM_WIN32_KHR 1
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#ifndef _DEBUG
#pragma comment( linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup" )
#endif

#elif _XCB // don't know how to detect this...

#define VK_USE_PLATFORM_XCB_KHR 1

#else
#error No platform specified
#endif

#if BUILD_ENABLE_CPP_DEBUG == 0
#define NDEBUG
#endif

#include <vulkan/vulkan.h>
