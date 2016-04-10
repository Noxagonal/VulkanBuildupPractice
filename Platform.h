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

#define PLATFORM_DEPENDENT_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME

#elif __linux

// for simplicity
// there are other ways to create windows on linux (wayland and mir)
#define VK_USE_PLATFORM_XCB_KHR 1

#define PLATFORM_DEPENDENT_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME

#else
#error No platform specified
#endif

#if BUILD_ENABLE_CPP_DEBUG == 0
#define NDEBUG
#endif

#include <vulkan/vulkan.h>
