
#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Shared.hpp"
#include "Mesh.h"


Mesh::Mesh()
{
}


Mesh::~Mesh()
{
}

void Mesh::CreateShape_Triangle()
{
	_vertices.resize( 3 );
	_indices.resize( 1 );

	_vertices[ 0 ].loc[ 0 ]			= 0.0f;
	_vertices[ 0 ].loc[ 1 ]			= -1.0f;
	_vertices[ 0 ].loc[ 2 ]			= 0.5f;/*
	_vertices[ 0 ].color[ 0 ]		= 1.0f;
	_vertices[ 0 ].color[ 1 ]		= 0.0f;
	_vertices[ 0 ].color[ 2 ]		= 0.0f;
	_vertices[ 0 ].color[ 3 ]		= 1.0f;
	_vertices[ 0 ].uv[ 0 ]			= 0.0f;
	_vertices[ 0 ].uv[ 1 ]			= 0.0f;*/

	_vertices[ 1 ].loc[ 0 ]			= -1.0f;
	_vertices[ 1 ].loc[ 1 ]			= 1.0f;
	_vertices[ 1 ].loc[ 2 ]			= 0.5f;/*
	_vertices[ 1 ].color[ 0 ]		= 0.0f;
	_vertices[ 1 ].color[ 1 ]		= 1.0f;
	_vertices[ 1 ].color[ 2 ]		= 0.0f;
	_vertices[ 1 ].color[ 3 ]		= 1.0f;
	_vertices[ 1 ].uv[ 0 ]			= 0.0f;
	_vertices[ 1 ].uv[ 1 ]			= 0.0f;*/

	_vertices[ 2 ].loc[ 0 ]			= 1.0f;
	_vertices[ 2 ].loc[ 1 ]			= 1.0f;
	_vertices[ 2 ].loc[ 2 ]			= 0.5f;/*
	_vertices[ 2 ].color[ 0 ]		= 0.0f;
	_vertices[ 2 ].color[ 1 ]		= 0.0f;
	_vertices[ 2 ].color[ 2 ]		= 1.0f;
	_vertices[ 2 ].color[ 3 ]		= 1.0f;
	_vertices[ 2 ].uv[ 0 ]			= 0.0f;
	_vertices[ 2 ].uv[ 1 ]			= 0.0f;*/

	_indices[ 0 ].vertex_ids[ 0 ]	= 0;
	_indices[ 0 ].vertex_ids[ 1 ]	= 1;
	_indices[ 0 ].vertex_ids[ 2 ]	= 2;
}

const std::vector<Mesh_Vertex>* Mesh::GetVerticesList()
{
	return &_vertices;
}

const std::vector<Mesh_Polygon>* Mesh::GetIndicesList()
{
	return &_indices;
}

uint64_t Mesh::GetVerticesListByteSize()
{
	return _vertices.size() * sizeof( Mesh_Vertex );
}

uint64_t Mesh::GetIndicesListByteSize()
{
	return _indices.size() * sizeof( Mesh_Polygon );
}
