#pragma once

#include <vector>

#include "Mesh.h"
#include "SceneObject.h"

class Mesh;

class SO_DynamicMesh : public SceneObject
{
public:
	SO_DynamicMesh( Renderer * renderer, Mesh * mesh );
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
