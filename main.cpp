#include "Shared.hpp"
#include "Renderer.h"
#include "Window.h"
#include "Pipeline.h"
#include "Scene.h"
#include "SO_DynamicMesh.h"
#include "Mesh.h"

#include <assert.h>
#include <iostream>

#define _USE_MATH_DEFINES
#include <math.h>

constexpr uint32_t TRIANGLE_COUNT = 20;

int main()
{
<<<<<<< HEAD
	std::vector<std::string> pipeline_names {
		"default"
	};
	Renderer renderer( pipeline_names );
	Window		*	window		= renderer.OpenWindow( { 800, 600 }, "test" );
	Scene		*	scene		= renderer.CreateScene();

	Mesh triangle;
	triangle.CreateShape_Triangle();

	std::vector<SO_DynamicMesh*> sobj( TRIANGLE_COUNT );		// list of dynamic meshes
	std::vector<float> sobj_rot_diff( sobj.size() );			// rotational difference between animations on meshes.
	for( uint32_t i=0; i < sobj.size(); ++i ) {
		sobj[ i ] 					= scene->CreateSceneObject_DynamicMesh( &triangle );
		sobj[ i ]->SetActiveWindow( window );								// set active window, trying to get rid of this step
		sobj[ i ]->SetActivePipeline( window->GetPipelines()[ 0 ] );		// set active pipeline, this step is required but there might be a lot nicer way of doing it
		sobj_rot_diff[ i ]			= float( i * M_PI * 2 * 0.01 );			// last value is in "circles", 1.0f equals one full round per object.
	}

	float rotator = 0.0f;		// simple ever increasing float

	while( renderer.Run() ) {
		rotator += 0.0006f;		// increasing the "float counter". This just moves the vertices around a little

		// update meshes manually, ideally this would be it's own entity with a link to a scene_object.
		for( uint32_t i=0; i < sobj.size(); ++i ) {
			sobj[ i ]->GetEditableVertices()[ 0 ].loc[ 0 ]		= cos( rotator + sobj_rot_diff[ i ] ) / 2.0f;	// x
			sobj[ i ]->GetEditableVertices()[ 0 ].loc[ 1 ]		= sin( rotator + sobj_rot_diff[ i ] ) / 2.0f;	// y
		}

		scene->Update();					// update scene, this handles all general stuff for now including vertex uploads to GPU, this is recursive
		window->RenderScene( scene );		// render scene, this is also recursive
	}
	vkQueueWaitIdle( renderer.GetVulkanQueue() );

	return 0;
=======
    std::vector<std::string> pipeline_names {
        "default"
    };
    Renderer renderer( pipeline_names );
    Window		*	window		= renderer.OpenWindow( { 800, 600 }, "test" );
    Scene		*	scene		= renderer.CreateScene();

    Mesh triangle;
    triangle.CreateShape_Triangle();

    std::vector<SO_DynamicMesh*> sobj( TRIANGLE_COUNT );		// list of dynamic meshes
    std::vector<float> sobj_rot_diff( sobj.size() );			// rotational difference between animations on meshes.
    for( uint32_t i=0; i < sobj.size(); ++i ) {
        sobj[ i ] 					= scene->CreateSceneObject_DynamicMesh( &triangle );
        sobj[ i ]->SetActiveWindow( window );
        sobj[ i ]->SetActivePipeline( window->GetPipelines()[ 0 ] );
        sobj_rot_diff[ i ]			= i * M_PI * 2  * 0.01f;	// last value is in "circles", 1.0f equals one full round per object.
    }

    float rotator = 0.0f;

    while( renderer.Run() ) {
        rotator += 0.0006f;

        std::vector<VkCommandBuffer> cmd_buffers( sobj.size() );
        for( uint32_t i=0; i < sobj.size(); ++i ) {
            sobj[ i ]->GetEditableVertices()[ 0 ].loc[ 0 ]		= cos( rotator + sobj_rot_diff[ i ] ) / 2.0f;
            sobj[ i ]->GetEditableVertices()[ 0 ].loc[ 1 ]		= sin( rotator + sobj_rot_diff[ i ] ) / 2.0f;
            sobj[ i ]->Update();
            cmd_buffers[ i ]									= sobj[ i ]->GetActiveCommandBuffer();
        }

        window->Render( cmd_buffers );
    }
    vkQueueWaitIdle( renderer.GetVulkanQueue() );

    return 0;
>>>>>>> refs/remotes/origin/XcbWindowHandling
}
