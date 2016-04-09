
#include "Scene.h"

#include "SO_DynamicMesh.h"

Scene::Scene( Renderer * renderer )
{
	_renderer		= renderer;
}

Scene::~Scene()
{
	for( auto obj : _scene_objects ) {
		delete obj;
	}
}

SO_DynamicMesh * Scene::CreateSceneObject_DynamicMesh( Mesh * mesh )
{
	auto obj = new SO_DynamicMesh( _renderer, mesh );
	_scene_objects.push_back( obj );
	return obj;
}
