#pragma once

#include "BUILD_OPTIONS.h"

#ifdef _WIN32
// _WIN32 is always defined on Windows 32 and 64 bit

// include platform specific headers
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

// define platform for Vulkan
#define VK_USE_PLATFORM_WIN32_KHR 1
#define PLATFORM_DEPENDENT_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME

#ifndef _DEBUG
#pragma comment( linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup" )
#endif

#elif __linux
// for simplicity
// there are other ways to create windows on linux (wayland and mir)

// include platform specific headers
#include <xcb/xcb.h>

// define platform for Vulkan
#define VK_USE_PLATFORM_XCB_KHR 1
#define PLATFORM_DEPENDENT_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME

#else
// Your current platform isn't supported in this code
#error Platform not supported
#endif

#if BUILD_ENABLE_CPP_DEBUG == 0
#define NDEBUG
#endif

#include <vulkan/vulkan.h>
