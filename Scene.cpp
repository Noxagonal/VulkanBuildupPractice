
#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Shared.hpp"
#include "Scene.h"

#include "SO_DynamicMesh.h"

Scene::Scene( Scene * parent_scene, Renderer * renderer )
{
	_parent			= parent_scene;
	_renderer		= renderer;
}

Scene::~Scene()
{
	for( auto obj : _scene_objects ) {
		delete obj;
	}
	for( auto sce : _child_scenes ) {
		delete sce;
	}
}

void Scene::Update()
{
	for( auto obj : _scene_objects ) {
		obj->Update();
	}
	for( auto sce : _child_scenes ) {
		sce->Update();
	}
}

Scene * Scene::CreateChildScene()
{
	Scene * child = new Scene( this, _renderer );
	_child_scenes.push_back( child );
	return child;
}

SO_DynamicMesh * Scene::CreateSceneObject_DynamicMesh( Mesh * mesh )
{
	auto obj = new SO_DynamicMesh( this, _renderer, mesh );
	_scene_objects.push_back( obj );
	return obj;
}

void Scene::CollectCommandBuffers_Local( std::vector<VkCommandBuffer>& out_command_buffers, bool force_recalculate ) const
{
	for( auto obj : _scene_objects ) {
		out_command_buffers.push_back( obj->GetActiveCommandBuffer( force_recalculate ) );
	}
}

void Scene::CollectCommandBuffers_Recursive( std::vector<VkCommandBuffer>& out_command_buffers, bool force_recalculate ) const
{
	CollectCommandBuffers_Local( out_command_buffers, force_recalculate );
	for( auto sce : _child_scenes ) {
		sce->CollectCommandBuffers_Recursive( out_command_buffers, force_recalculate );
	}
}
