#pragma once

#include "Platform.h"
#include <vector>
#include <list>

class Renderer;
class SceneObject;

class Mesh;

class SO_DynamicMesh;

class Scene
{
public:
	Scene( Scene * parent_scene, Renderer * renderer );
	~Scene();

	void							Update();

	Scene						*	CreateChildScene();

	SO_DynamicMesh				*	CreateSceneObject_DynamicMesh( Mesh * mesh );

	void							CollectCommandBuffers_Local( std::vector<VkCommandBuffer> & out_command_buffers, bool force_recalculate = false ) const;
	void							CollectCommandBuffers_Recursive( std::vector<VkCommandBuffer> & out_command_buffers, bool force_recalculate = false ) const;

private:
	Renderer					*	_renderer				= nullptr;
	Scene						*	_parent					= nullptr;

	std::list<Scene*>				_child_scenes;
	std::list<SceneObject*>			_scene_objects;
};
