#pragma once

#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Mesh.h"
#include "SceneObject.h"

#include <vector>

class Mesh;
class Scene;

// SceneObject of DynamicMesh variety. This class has it's own copy of the data
// which can be updated without modifying the original mesh.
class SO_DynamicMesh : public SceneObject
{
public:
	SO_DynamicMesh( Scene * parent_scene, Renderer * renderer, Mesh * mesh );
	~SO_DynamicMesh();

	void									Update();
	const std::vector<Mesh_Vertex>		&	GetVertices() const;
	std::vector<Mesh_Vertex>			&	GetEditableVertices();
	const std::vector<Mesh_Polygon>		&	GetIndeces() const;
	std::vector<Mesh_Polygon>			&	GetEditableIndices();

protected:
	void									_Initialize();
	void									_RebuildCommandBuffer();

private:
	Mesh								*	_mesh;
	std::vector<Mesh_Vertex>				_local_vertices;
	std::vector<Mesh_Polygon>				_local_indices;

	VkBuffer								_vertex_buffer				= VK_NULL_HANDLE;
	VkBuffer								_index_buffer				= VK_NULL_HANDLE;
	uint64_t								_vertex_buffer_size			= 0;
	uint64_t								_index_buffer_size			= 0;
	VkDeviceMemory							_vertex_buffer_memory		= VK_NULL_HANDLE;
	VkDeviceMemory							_index_buffer_memory		= VK_NULL_HANDLE;
};
