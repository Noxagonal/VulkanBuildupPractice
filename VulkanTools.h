#pragma once

#include "BUILD_OPTIONS.h"
#include "Platform.h"
#include "Shared.hpp"

#include "VulkanCollections.h"

#include <vector>

class Renderer;

void FindBufferMemoryType( Renderer * renderer, Buffer & buffer );

void AllocateBuffersMemory( Renderer * renderer, std::vector<Buffer> & buffers );
void FreeBuffersMemory( Renderer * renderer, std::vector<Buffer> & buffers );
