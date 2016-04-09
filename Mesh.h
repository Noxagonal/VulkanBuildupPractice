#pragma once

#include <vector>
#include <cstdint>

struct Mesh_Vertex
{
	float loc[ 3 ];
//	float color[ 4 ];
//	float uv[ 2 ];
};

struct Mesh_Polygon
{
	uint32_t vertex_ids[ 3 ];
};

class Mesh
{
public:
	Mesh();
	~Mesh();

	void		Load( std::string filepath );

	void		CreateShape_Triangle();

	const std::vector<Mesh_Vertex>		*	GetVerticesList();
	const std::vector<Mesh_Polygon>		*	GetIndicesList();

	uint64_t								GetVerticesListByteSize();
	uint64_t								GetIndicesListByteSize();

private:
	std::vector<Mesh_Vertex>				_vertices;
	std::vector<Mesh_Polygon>				_indices;
};

