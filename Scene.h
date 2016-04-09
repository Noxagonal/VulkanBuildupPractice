#pragma once

#include <vector>

class Renderer;
class SceneObject;

class Mesh;

class SO_DynamicMesh;

class Scene
{
public:
	Scene( Renderer * renderer );
	~Scene();

	SO_DynamicMesh				*	CreateSceneObject_DynamicMesh( Mesh * mesh );

private:
	Renderer					*	_renderer				= nullptr;

	std::vector<SceneObject*>		_scene_objects;
};
