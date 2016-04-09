
#include "BUILD_OPTIONS.h"
#include "Window.h"
#include "Renderer.h"
#include "Pipeline.h"
#include "Scene.h"

#include <algorithm>
#include <assert.h>

Window::Window( Renderer * renderer, VkExtent2D dimensions, std::string window_name )
{
	_swapchain_image_count		= 2;		// 2 = double buffering, 3 = triple buffering
	_renderer					= renderer;
	_window_name				= window_name;
	_device						= renderer->_device;
	_queue						= renderer->_queue;

	_SubConstructor( dimensions );
}


Window::~Window()
{
	_SubDestructor();
}

void Window::_SubConstructor( VkExtent2D dimensions )
{
	_surface_size		= dimensions;

	_CreateSetupCommandPool();
	_AllocateSetupCommandBuffer();
	_BeginSetupCommandBuffer();

	_CreateOSWindow();
	_CreateSurface();
	_CreateSwapchain();
	_CreateSwapchainImages();
	_CreateDepthBuffer();
	_CreateRenderPass();
	_CreateFrameBuffers();

	_EndSetupCommandBuffer();
	_ExecuteSetupCommandBuffer();

	_CreateRenderCommands();

	_CreatePipelines();

	ErrCheck( vkAcquireNextImageKHR( _device, _swapchain, UINT64_MAX, _present_image_available, VK_NULL_HANDLE, &_current_swapchain_image ) );
}

void Window::_SubDestructor()
{
	vkQueueWaitIdle( _queue );

	_DestroyPipelines();

	_DestroyRenderCommands();
	_DestroyFrameBuffers();
	_DestroyRenderPass();
	_DestroyDepthBuffer();
	_DestroySwapchainImages();
	_DestroySetupCommandPool();
	_DestroySwapchain();
	_DestroySurface();
	_DestroyOSWindow();
}

void Window::Update()
{
	_UpdateOSWindow();
}

void Window::Close()
{
	_window_should_close = true;
}

void Window::Render( const std::vector<VkCommandBuffer> & command_buffers )
{
	// Trying 2 pipeline barriers inside one command buffer
	// This seems to work pretty well on my system
	VkCommandBufferBeginInfo command_buffer_begin_info {};
	command_buffer_begin_info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags				= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	ErrCheck( vkBeginCommandBuffer( _render_command_buffers[ _current_swapchain_image ], &command_buffer_begin_info ) );
	{
		// memory barrier to transfer image from presentable to writeable
		VkImageMemoryBarrier image_barrier {};
		image_barrier.sType							= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_barrier.image							= _swapchain_images[ _current_swapchain_image ];
		image_barrier.oldLayout						= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		image_barrier.newLayout						= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_barrier.srcAccessMask					= VK_ACCESS_MEMORY_READ_BIT;
		image_barrier.dstAccessMask					= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		image_barrier.srcQueueFamilyIndex			= VK_QUEUE_FAMILY_IGNORED;
		image_barrier.dstQueueFamilyIndex			= VK_QUEUE_FAMILY_IGNORED;
		image_barrier.subresourceRange.aspectMask			= VK_IMAGE_ASPECT_COLOR_BIT;
		image_barrier.subresourceRange.layerCount			= 1;
		image_barrier.subresourceRange.levelCount			= 1;
		image_barrier.subresourceRange.baseArrayLayer		= 0;
		image_barrier.subresourceRange.baseMipLevel			= 0;

		vkCmdPipelineBarrier(
			_render_command_buffers[ _current_swapchain_image ],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier );
	}

	VkRect2D render_area;
	render_area.offset.x		= 0;
	render_area.offset.y		= 0;
	render_area.extent			= _surface_size;

	VkClearValue clear_values[ 2 ];
	clear_values[ 0 ].depthStencil.depth	= 1.0f;
	clear_values[ 0 ].depthStencil.stencil	= 0;
	clear_values[ 1 ].color.float32[ 0 ]	= 0.10f;
	clear_values[ 1 ].color.float32[ 1 ]	= 0.15f;
	clear_values[ 1 ].color.float32[ 2 ]	= 0.20f;
	clear_values[ 1 ].color.float32[ 3 ]	= 1.0f;

	VkRenderPassBeginInfo begin_info {};
	begin_info.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	begin_info.renderPass		= _render_pass;
	begin_info.framebuffer		= _framebuffers[ _current_swapchain_image ];
	begin_info.renderArea		= render_area;
	begin_info.clearValueCount	= 2;
	begin_info.pClearValues		= clear_values;
	vkCmdBeginRenderPass( _render_command_buffers[ _current_swapchain_image ], &begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS );
	// objects render here
	vkCmdExecuteCommands( _render_command_buffers[ _current_swapchain_image ], command_buffers.size(), command_buffers.data() );
	vkCmdEndRenderPass( _render_command_buffers[ _current_swapchain_image ] );

	{
		// memory barrier to transfer image from writeable to presentable
		VkImageMemoryBarrier image_barrier {};
		image_barrier.sType						= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_barrier.image						= _swapchain_images[ _current_swapchain_image ];
		image_barrier.oldLayout					= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_barrier.newLayout					= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		image_barrier.srcAccessMask				= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		image_barrier.dstAccessMask				= VK_ACCESS_MEMORY_READ_BIT;
		image_barrier.srcQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
		image_barrier.dstQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
		image_barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		image_barrier.subresourceRange.layerCount		= 1;
		image_barrier.subresourceRange.levelCount		= 1;
		image_barrier.subresourceRange.baseArrayLayer	= 0;
		image_barrier.subresourceRange.baseMipLevel		= 0;

		vkCmdPipelineBarrier(
			_render_command_buffers[ _current_swapchain_image ],
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier );
	}

	ErrCheck( vkEndCommandBuffer( _render_command_buffers[ _current_swapchain_image ] ) );

	VkPipelineStageFlags stage_flags[] { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	VkSubmitInfo submit_info {};
	submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount		= 1;
	submit_info.pCommandBuffers			= &_render_command_buffers[ _current_swapchain_image ];
	submit_info.waitSemaphoreCount		= 1;
	submit_info.pWaitSemaphores			= &_present_image_available;
	submit_info.pWaitDstStageMask		= stage_flags;
	submit_info.signalSemaphoreCount	= 1;
	submit_info.pSignalSemaphores		= &_render_complete[ _current_swapchain_image ];

	ErrCheck( vkQueueSubmit( _queue, 1, &submit_info, VK_NULL_HANDLE ) );

	VkPresentInfoKHR present_info {};
	present_info.sType					= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.swapchainCount			= 1;
	present_info.pSwapchains			= &_swapchain;
	present_info.pImageIndices			= &_current_swapchain_image;
	present_info.waitSemaphoreCount		= 1;
	present_info.pWaitSemaphores		= &_render_complete[ _current_swapchain_image ];
	ErrCheck( vkQueuePresentKHR( _queue, &present_info ) );

	ErrCheck( vkAcquireNextImageKHR( _device, _swapchain, UINT64_MAX, _present_image_available, VK_NULL_HANDLE, &_current_swapchain_image ) );

	// really simple syncronization, replace with something more sophisticated later
	vkQueueWaitIdle( _queue );
}

void Window::RenderScene( const Scene * scene, bool force_recalculate )
{
	std::vector<VkCommandBuffer> render_command_buffers;

	// do a recursive search on the scene and find all objects
	scene->CollectCommandBuffers_Recursive( render_command_buffers, force_recalculate );
	Render( render_command_buffers );
}

VkExtent2D Window::GetSize()
{
	return _surface_size;
}

const std::vector<Pipeline*> & Window::GetPipelines()
{
	return _pipelines;
}

Pipeline * Window::FindPipeline( std::string name )
{
	for( auto p : _pipelines ) {
		if( p->GetName() == name ) {
			return p;
		}
	}
	return nullptr;
}

VkRenderPass Window::GetRenderPass()
{
	return _render_pass;
}

const std::vector<VkFramebuffer> & Window::GetFrameBuffers()
{
	return _framebuffers;
}

uint32_t Window::GetCurrentFrameBufferIndex()
{
	return _current_swapchain_image;
}

void Window::Resize( VkExtent2D size )
{
	_SubDestructor();
	_SubConstructor( size );
}


void Window::_CreateSetupCommandPool()
{
	VkCommandPoolCreateInfo command_pool_create_info {};
	command_pool_create_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.queueFamilyIndex	= _renderer->_render_queue_family_index;
	command_pool_create_info.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	ErrCheck( vkCreateCommandPool( _renderer->_device, &command_pool_create_info, nullptr, &_setup_command_pool ) );
	assert( _setup_command_pool );
}

void Window::_DestroySetupCommandPool()
{
	vkDestroyCommandPool( _renderer->_device, _setup_command_pool, nullptr );
}

void Window::_AllocateSetupCommandBuffer()
{
	VkCommandBufferAllocateInfo command_buffer_alloc_info {};
	command_buffer_alloc_info.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_alloc_info.commandPool			= _setup_command_pool;
	command_buffer_alloc_info.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_alloc_info.commandBufferCount	= 1;

	ErrCheck( vkAllocateCommandBuffers( _device, &command_buffer_alloc_info, &_setup_command_buffer ) );
	assert( _setup_command_buffer );
}

void Window::_BeginSetupCommandBuffer()
{
	VkCommandBufferBeginInfo command_buffer_begin_info {};
	command_buffer_begin_info.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	ErrCheck( vkBeginCommandBuffer( _setup_command_buffer, &command_buffer_begin_info ) );
}

void Window::_EndSetupCommandBuffer()
{
	ErrCheck( vkEndCommandBuffer( _setup_command_buffer ) );
}

void Window::_ExecuteSetupCommandBuffer()
{
	VkFenceCreateInfo fence_create_info {};
	fence_create_info.sType			= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkFence fence					= VK_NULL_HANDLE;
	vkCreateFence( _device, &fence_create_info, nullptr, &fence );

	VkSubmitInfo submit_info {};
	submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount	= 1;
	submit_info.pCommandBuffers		= &_setup_command_buffer;

	vkQueueSubmit( _renderer->_queue, 1, &submit_info, fence );

	VkResult err = vkWaitForFences( _device, 1, &fence, true, UINT64_MAX );
	if( VK_TIMEOUT == err ) {
		assert( 0 && "Vulkan ERROR: Queue submit fence timeout." );
		std::exit( -1 );
	}

	vkDestroyFence( _device, fence, nullptr );
}


#if VK_USE_PLATFORM_WIN32_KHR

// MS-Windows event handling function:
LRESULT CALLBACK WindowsEventHandler( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	Window * window = reinterpret_cast<Window*>(
		GetWindowLongPtrW( hWnd, GWLP_USERDATA ) );

	switch( uMsg ) {
	case WM_CLOSE:
		window->Close();
		break;
	case WM_SIZE:
		// we get here if the window has changed size, we should rebuild most
		// of our window resources before rendering to this window again.
		// ( no need for this because our window sizing by hand is disabled )
		break;
	default:
		return ( DefWindowProc( hWnd, uMsg, wParam, lParam ) );
	}
	return 0;
}

uint64_t Window::_win32_class_id_counter = 0;

void Window::_CreateOSWindow()
{
	WNDCLASSEX win_class {};
	assert( _surface_size.width > 0 );
	assert( _surface_size.height > 0 );

	_win32_instance				= GetModuleHandle( nullptr );
	_win32_class_name			= _window_name + "_" + std::to_string( _win32_class_id_counter );
	++_win32_class_id_counter;

	// Initialize the window class structure:
	win_class.cbSize			= sizeof( WNDCLASSEX );
	win_class.style				= CS_HREDRAW | CS_VREDRAW;
	win_class.lpfnWndProc		= WindowsEventHandler;
	win_class.cbClsExtra		= 0;
	win_class.cbWndExtra		= 0;
	win_class.hInstance			= _win32_instance; // hInstance
	win_class.hIcon				= LoadIcon( NULL, IDI_APPLICATION );
	win_class.hCursor			= LoadCursor( NULL, IDC_ARROW );
	win_class.hbrBackground		= (HBRUSH)GetStockObject( WHITE_BRUSH );
	win_class.lpszMenuName		= NULL;
	win_class.lpszClassName		= _win32_class_name.c_str();
	win_class.hIconSm			= LoadIcon( NULL, IDI_WINLOGO );
	// Register window class:
	if( !RegisterClassEx( &win_class ) ) {
		// It didn't work, so try to give a useful error:
		assert( 0 && "Cannot create a window in which to draw!\n" );
		fflush( stdout );
		exit( -1 );
	}

	DWORD ex_style	= WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD style		= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX; // | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	// Create window with the registered class:
	RECT wr = { 0, 0, LONG( _surface_size.width ), LONG( _surface_size.height ) };
	AdjustWindowRectEx( &wr, style, FALSE, ex_style );
	_win32_window = CreateWindowEx( 0,
		_win32_class_name.c_str(),		// class name
		_window_name.c_str(),			// app name
		style,							// window style
		CW_USEDEFAULT, CW_USEDEFAULT,	// x/y coords
		wr.right - wr.left,				// width
		wr.bottom - wr.top,				// height
		NULL,							// handle to parent
		NULL,							// handle to menu
		_win32_instance,				// hInstance
		NULL );							// no extra parameters
	if( !_win32_window ) {
		// It didn't work, so try to give a useful error:
		assert( 1 && "Cannot create a window in which to draw!\n" );
		fflush( stdout );
		exit( -1 );
	}
	SetWindowLongPtr( _win32_window, GWLP_USERDATA, (LONG_PTR)this );

	ShowWindow( _win32_window, SW_SHOW );
	SetForegroundWindow( _win32_window );
	SetFocus( _win32_window );

//	RECT window_dimensions {};
//	GetWindowRect( _win32_window, &window_dimensions );
//	_dimensions.width		= window_dimensions.right - window_dimensions.left;
//	_dimensions.height		= window_dimensions.bottom - window_dimensions.top;
}

void Window::_DestroyOSWindow()
{
	DestroyWindow( _win32_window );
	UnregisterClass( _win32_class_name.c_str(), _win32_instance );
	_win32_window		= nullptr;
}


#elif VK_USE_PLATFORM_XCB_KHR


void Window::_CreateOSWindow()
{
	// create connection
	const xcb_setup_t		*	setup		= nullptr;
	xcb_screen_iterator_t		iter;
	int							screen		= 0;

	_xcb_connection			=	xcb_connect( nullptr, &screen );
	if( _xcb_connection == nullptr ) {
		std::cout << "Cannot find a compatible Vulkan ICD.\n";
		exit( -1 );
	}

	setup		= xcb_get_setup( _xcb_connection );
	iter		= xcb_setup_roots_iterator( setup );
	while( screen-- > 0 ) {
		xcb_screen_next( &iter );
	}
	_xcb_screen = iter.data;

	// create window
	assert( _dimensions.width > 0 );
	assert( _dimensions.height > 0 );

	uint32_t value_mask, value_list[ 32 ];

	_xcb_window = xcb_generate_id( _xcb_connection );

	value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value_list[ 0 ] = _xcb_screen->black_pixel;
	value_list[ 1 ] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE;

	xcb_create_window( _xcb_connection, XCB_COPY_FROM_PARENT, _xcb_window,
		_xcb_screen->root, 0, 0, _dimensions.width, _dimensions.height, 0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT, _xcb_screen->root_visual,
		value_mask, value_list );

	/* Magic code that will send notification when window is destroyed */
	// might cause problems because I put the connection in the renderer... Lets hope this works
	xcb_intern_atom_cookie_t cookie =
		xcb_intern_atom( _xcb_connection, 1, 12, "WM_PROTOCOLS" );
	xcb_intern_atom_reply_t *reply =
		xcb_intern_atom_reply( _xcb_connection, cookie, 0 );

	xcb_intern_atom_cookie_t cookie2 =
		xcb_intern_atom( _xcb_connection, 0, 16, "WM_DELETE_WINDOW" );
	_xcb_atom_window_reply =
		xcb_intern_atom_reply( _xcb_connection, cookie2, 0 );

	xcb_change_property( _xcb_connection, XCB_PROP_MODE_REPLACE, _xcb_window,
		( *reply ).atom, 4, 32, 1,
		&( *_xcb_atom_window_reply ).atom );
	free( reply );

	xcb_map_window( _xcb_connection, _xcb_window );

	// Force the x/y coordinates to 100,100 results are identical in consecutive
	// runs
	const uint32_t coords[] = { 100, 100 };
	xcb_configure_window( _xcb_connection, _xcb_window,
		XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords );
	xcb_flush( _xcb_connection );

	xcb_generic_event_t *e;
	while( ( e = xcb_wait_for_event( _xcb_connection ) ) ) {
		if( ( e->response_type & ~0x80 ) == XCB_EXPOSE )
			break;
	}
}

void Window::_DestroyOSWindow()
{
	xcb_destroy_window( _xcb_connection, _xcb_window );
	xcb_disconnect( _xcb_connection );
	_xcb_window			= nullptr;
	_xcb_connection		= nullptr;
}


#endif

void Window::_CreateSurface()
{
#if VK_USE_PLATFORM_WIN32_KHR
	VkWin32SurfaceCreateInfoKHR create_info {};
	create_info.sType			= VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	create_info.hinstance		= _win32_instance;
	create_info.hwnd			= _win32_window;
	ErrCheck( vkCreateWin32SurfaceKHR( _renderer->_instance, &create_info, nullptr, &_surface ) );
#elif VK_USE_PLATFORM_XCB_KHR
	VkXcbSurfaceCreateInfoKHR create_info {};
	create_info.sType			= VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	create_info.connection		= _xcb_connection;
	create_info.window			= _xcb_window;
	ErrCheck( vkCreateXcbSurfaceKHR( _renderer->_instance, &create_info, nullptr, &_surface ) );
#endif

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( _renderer->_gpu, _surface, &_surface_capabilities );
	{
		uint32_t format_count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR( _renderer->_gpu, _surface, &format_count, nullptr );
		std::vector<VkSurfaceFormatKHR> format_list( format_count );
		vkGetPhysicalDeviceSurfaceFormatsKHR( _renderer->_gpu, _surface, &format_count, format_list.data() );
		if( format_list.size() == 1 && format_list[ 0 ].format == VK_FORMAT_UNDEFINED ) {
			_surface_format.format			= VK_FORMAT_B8G8R8A8_UNORM;
			_surface_format.colorSpace		= VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		} else {
			_surface_format					= format_list[ 0 ];
		}
	}
	ErrCheck( vkGetPhysicalDeviceSurfaceSupportKHR( _renderer->_gpu, _renderer->_render_queue_family_index, _surface, &_WSI_supported ) );
	if( !_WSI_supported ) {
		assert( 0 && "Vulkan ERROR: Surface doesn't have WSI support." );
		std::exit( -1 );
	}
}

void Window::_DestroySurface()
{
	vkDestroySurfaceKHR( _renderer->_instance, _surface, nullptr );
	_surface = VK_NULL_HANDLE;
}

void Window::_CreateSwapchain()
{
	// select swapchain image amount
	_swapchain_image_count			= std::min( std::max( _swapchain_image_count, _surface_capabilities.minImageCount ), _surface_capabilities.maxImageCount );

	// make sure that the swapchain images and the surface area match in size
	// checking only width is enough
	if( _surface_capabilities.currentExtent.width < UINT32_MAX ) {
		_surface_size.width			= _surface_capabilities.currentExtent.width;
		_surface_size.height		= _surface_capabilities.currentExtent.height;
	}

	// select present mode i.e vertical sync
	VkPresentModeKHR present_mode		= VK_PRESENT_MODE_FIFO_KHR;
	{
		uint32_t present_mode_count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR( _renderer->_gpu, _surface, &present_mode_count, nullptr );
		std::vector<VkPresentModeKHR> present_mode_list( present_mode_count );
		vkGetPhysicalDeviceSurfacePresentModesKHR( _renderer->_gpu, _surface, &present_mode_count, present_mode_list.data() );
		uint32_t aflags = 0;
		for( auto i : present_mode_list ) {
			if( i == VK_PRESENT_MODE_MAILBOX_KHR )		aflags |= 1;
			if( i == VK_PRESENT_MODE_IMMEDIATE_KHR )	aflags |= 2;
		}
		if		( aflags & 1 )		present_mode		= VK_PRESENT_MODE_MAILBOX_KHR;
		else if	( aflags & 2 )		present_mode		= VK_PRESENT_MODE_IMMEDIATE_KHR;
	}

	VkSwapchainCreateInfoKHR create_info {};
	create_info.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface					= _surface;
	create_info.minImageCount			= _swapchain_image_count;
	create_info.imageFormat				= _surface_format.format;
	create_info.imageExtent				= _surface_size;
	create_info.preTransform			= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	create_info.compositeAlpha			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.imageArrayLayers		= 1;
	create_info.presentMode				= present_mode;
	create_info.clipped					= true;
	create_info.imageColorSpace			= _surface_format.colorSpace;
	create_info.imageUsage				= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	create_info.imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount	= 0;
	create_info.pQueueFamilyIndices		= nullptr;

	ErrCheck( vkCreateSwapchainKHR( _renderer->_device, &create_info, nullptr, &_swapchain ) );
}

void Window::_DestroySwapchain()
{
	vkDestroySwapchainKHR( _renderer->_device, _swapchain, nullptr );
	_swapchain = VK_NULL_HANDLE;
}

void Window::_CreateSwapchainImages()
{
	uint32_t swapchain_image_count = 0;
	{
		vkGetSwapchainImagesKHR( _renderer->_device, _swapchain, &swapchain_image_count, nullptr );
		_swapchain_images.resize( swapchain_image_count );
		_swapchain_image_views.resize( swapchain_image_count );
		vkGetSwapchainImagesKHR( _renderer->_device, _swapchain, &swapchain_image_count, _swapchain_images.data() );
		assert( swapchain_image_count );
	}
	for( uint32_t i=0; i < swapchain_image_count; ++i ) {
		auto &image		= _swapchain_images[ i ];

		VkImageViewCreateInfo view_create_info {};
		view_create_info.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_create_info.viewType		= VK_IMAGE_VIEW_TYPE_2D;
		view_create_info.image			= image;
		view_create_info.format			= _surface_format.format;
		view_create_info.components.r	= VK_COMPONENT_SWIZZLE_R;
		view_create_info.components.g	= VK_COMPONENT_SWIZZLE_G;
		view_create_info.components.b	= VK_COMPONENT_SWIZZLE_B;
		view_create_info.components.a	= VK_COMPONENT_SWIZZLE_A;
		view_create_info.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		view_create_info.subresourceRange.levelCount		= 1;
		view_create_info.subresourceRange.layerCount		= 1;
		view_create_info.subresourceRange.baseMipLevel		= 0;
		view_create_info.subresourceRange.baseArrayLayer	= 0;

		ErrCheck( vkCreateImageView( _renderer->_device, &view_create_info, nullptr, &_swapchain_image_views[ i ] ) );

		VkImageMemoryBarrier image_mem_barrier {};
		image_mem_barrier.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_mem_barrier.image					= image;
		image_mem_barrier.oldLayout				= VK_IMAGE_LAYOUT_UNDEFINED;
		image_mem_barrier.newLayout				= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		image_mem_barrier.srcAccessMask			= 0;
		image_mem_barrier.dstAccessMask			= VK_ACCESS_MEMORY_READ_BIT;
		image_mem_barrier.subresourceRange		= view_create_info.subresourceRange;

		vkCmdPipelineBarrier( _setup_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_mem_barrier );
	}
}

void Window::_DestroySwapchainImages()
{
	for( uint32_t i=0; i < _swapchain_images.size(); ++i ) {
		vkDestroyImageView( _renderer->_device, _swapchain_image_views[ i ], nullptr );
	}
}

void Window::_CreateDepthBuffer()
{
	std::vector<VkFormat> try_depth_formats { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT };
	for( auto f : try_depth_formats ) {
		VkFormatProperties format_properties {};
		vkGetPhysicalDeviceFormatProperties( _renderer->_gpu, f, &format_properties );
		// notice "optimalTilingFeatures". We require depth stencil attachments on optimal tiling
		if( format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
			_depth_format			= f;
			break;
		}
	}
	if( !(_depth_format == VK_FORMAT_D16_UNORM_S8_UINT ||
		_depth_format == VK_FORMAT_D24_UNORM_S8_UINT ||
		_depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT ) ) {
		assert( 0 && "Format has no stencil." );
		std::exit( -1 );
	}
	if( VK_FORMAT_UNDEFINED == _depth_format ) {
		assert( 0 && "Vulkan ERROR: Required depth format not supported." );
		std::exit( -1 );
	}

	VkImageCreateInfo image_create_info {};
	image_create_info.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType				= VK_IMAGE_TYPE_2D;
	image_create_info.format				= _depth_format;
	image_create_info.extent.width			= _surface_size.width;
	image_create_info.extent.height			= _surface_size.height;
	image_create_info.extent.depth			= 1;
	image_create_info.arrayLayers			= 1;
	image_create_info.mipLevels				= 1;
	image_create_info.samples				= VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling				= VK_IMAGE_TILING_OPTIMAL;
	image_create_info.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.usage					= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	ErrCheck( vkCreateImage( _device, &image_create_info, nullptr, &_depth_image ) );

	VkMemoryRequirements memory_requirements {};
	vkGetImageMemoryRequirements( _device, _depth_image, &memory_requirements );
	memory_requirements.alignment;

	auto &mem_props = _renderer->_gpu_memory_properties;
	uint32_t support_bitfield	= memory_requirements.memoryTypeBits;
	uint32_t memory_type_index	= UINT_MAX;
	for( uint32_t i=0; i < mem_props.memoryTypeCount; ++i ) {
		if( support_bitfield & 1 ) {
			if( ( mem_props.memoryTypes[ i ].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) {
				memory_type_index	= i;
				break;
			}
		}
		support_bitfield >>= 1;
	}

	VkMemoryAllocateInfo allocate_info {};
	allocate_info.sType						= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize			= memory_requirements.size;
	allocate_info.memoryTypeIndex			= memory_type_index;

	ErrCheck( vkAllocateMemory( _device, &allocate_info, nullptr, &_depth_image_memory ) );
	ErrCheck( vkBindImageMemory( _device, _depth_image, _depth_image_memory, 0 ) );

	VkImageMemoryBarrier image_memory_barrier {};
	image_memory_barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.image				= _depth_image;
	image_memory_barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier.newLayout			= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	image_memory_barrier.srcAccessMask		= 0;
	image_memory_barrier.dstAccessMask		= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	image_memory_barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	image_memory_barrier.subresourceRange.layerCount		= 1;
	image_memory_barrier.subresourceRange.levelCount		= 1;
	image_memory_barrier.subresourceRange.baseArrayLayer	= 0;
	image_memory_barrier.subresourceRange.baseMipLevel		= 0;

	vkCmdPipelineBarrier( _setup_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier );

	VkImageViewCreateInfo image_view_create_info {};
	image_view_create_info.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.image			= _depth_image;
	image_view_create_info.format			= _depth_format;
	image_view_create_info.viewType			= VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.components.r		= VK_COMPONENT_SWIZZLE_R;
	image_view_create_info.components.g		= VK_COMPONENT_SWIZZLE_G;
	image_view_create_info.components.b		= VK_COMPONENT_SWIZZLE_B;
	image_view_create_info.components.a		= VK_COMPONENT_SWIZZLE_A;
	image_view_create_info.subresourceRange.aspectMask			= VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	image_view_create_info.subresourceRange.layerCount			= 1;
	image_view_create_info.subresourceRange.levelCount			= 1;
	image_view_create_info.subresourceRange.baseArrayLayer		= 0;
	image_view_create_info.subresourceRange.baseMipLevel		= 0;

	ErrCheck( vkCreateImageView( _device, &image_view_create_info, nullptr, &_depth_image_view ) );
}

void Window::_DestroyDepthBuffer()
{
	vkDestroyImageView( _device, _depth_image_view, nullptr );
	vkDestroyImage( _device, _depth_image, nullptr );
	vkFreeMemory( _device, _depth_image_memory, nullptr );
}

void Window::_CreateRenderPass()
{
	VkAttachmentDescription attachments[ 2 ] { {}, {} };

	attachments[ 0 ].flags						= 0;
	attachments[ 0 ].format						= _depth_format;
	attachments[ 0 ].samples					= VK_SAMPLE_COUNT_1_BIT;
	attachments[ 0 ].loadOp						= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[ 0 ].storeOp					= VK_ATTACHMENT_STORE_OP_STORE;
	attachments[ 0 ].stencilLoadOp				= VK_ATTACHMENT_LOAD_OP_LOAD;
	attachments[ 0 ].stencilStoreOp				= VK_ATTACHMENT_STORE_OP_STORE;
	attachments[ 0 ].initialLayout				= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[ 0 ].finalLayout				= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments[ 1 ].flags						= 0;
	attachments[ 1 ].format						= _color_format;
	attachments[ 1 ].samples					= VK_SAMPLE_COUNT_1_BIT;
	attachments[ 1 ].loadOp						= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[ 1 ].storeOp					= VK_ATTACHMENT_STORE_OP_STORE;
	attachments[ 1 ].stencilLoadOp				= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[ 1 ].stencilStoreOp				= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[ 1 ].initialLayout				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[ 1 ].finalLayout				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref {};
	depth_attachment_ref.attachment				= 0;
	depth_attachment_ref.layout					= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_ref {};
	color_attachment_ref.attachment				= 1;
	color_attachment_ref.layout					= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_description {};
	subpass_description.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.colorAttachmentCount	= 1;	// layout(location=0)
	subpass_description.pColorAttachments		= &color_attachment_ref;
	subpass_description.pDepthStencilAttachment	= &depth_attachment_ref;

	VkRenderPassCreateInfo render_pass_create_info {};
	render_pass_create_info.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount		= 2;
	render_pass_create_info.pAttachments		= attachments;
	render_pass_create_info.subpassCount		= 1;
	render_pass_create_info.pSubpasses			= &subpass_description;

	ErrCheck( vkCreateRenderPass( _device, &render_pass_create_info, nullptr, &_render_pass ) );
}

void Window::_DestroyRenderPass()
{
	vkDestroyRenderPass( _device, _render_pass, nullptr );
}

void Window::_CreateFrameBuffers()
{
	uint32_t color_image_count = uint32_t( _swapchain_image_views.size() );
	_framebuffers.resize( color_image_count );

	for( uint32_t i=0; i < color_image_count; ++i ) {
		VkImageView attachments[ 2 ];
		attachments[ 0 ]	= _depth_image_view;
		attachments[ 1 ]	= _swapchain_image_views[ i ];

		VkFramebufferCreateInfo framebuffer_create_info {};
		framebuffer_create_info.sType				= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass			= _render_pass;
		framebuffer_create_info.attachmentCount		= 2;
		framebuffer_create_info.pAttachments		= attachments;
		framebuffer_create_info.width				= _surface_size.width;
		framebuffer_create_info.height				= _surface_size.height;
		framebuffer_create_info.layers				= 1;

		ErrCheck( vkCreateFramebuffer( _device, &framebuffer_create_info, nullptr, &_framebuffers[ i ] ) );
	}
}

void Window::_DestroyFrameBuffers()
{
	for( auto buffer : _framebuffers ) {
		vkDestroyFramebuffer( _device, buffer, nullptr );
	}
	_framebuffers.empty();
}

void Window::_CreateRenderCommands()
{
	VkCommandPoolCreateInfo pool_create_info {};
	pool_create_info.sType					= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.queueFamilyIndex		= _renderer->_render_queue_family_index;
	pool_create_info.flags					= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	ErrCheck( vkCreateCommandPool( _device, &pool_create_info, nullptr, &_command_pool ) );

	VkCommandBufferAllocateInfo allocate_info {};
	allocate_info.sType						= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandBufferCount		= _swapchain_image_count;
	allocate_info.commandPool				= _command_pool;
	allocate_info.level						= VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	_render_command_buffers.resize( _swapchain_image_count );
	ErrCheck( vkAllocateCommandBuffers( _device, &allocate_info, _render_command_buffers.data() ) );

	// create all commands buffers that do not change
	VkCommandBufferBeginInfo command_buffer_begin_info {};
	command_buffer_begin_info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkSemaphoreCreateInfo semaphore_create_info {};
	semaphore_create_info.sType				= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	_render_complete.resize( _swapchain_image_count );
	ErrCheck( vkCreateSemaphore( _device, &semaphore_create_info, nullptr, &_present_image_available ) );
	for( uint32_t i=0; i < _swapchain_image_count; ++i ) {
		ErrCheck( vkCreateSemaphore( _device, &semaphore_create_info, nullptr, &_render_complete[ i ] ) );
	}
}

void Window::_DestroyRenderCommands()
{
	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType			= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags			= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer( _render_command_buffers[ _current_swapchain_image ], &begin_info );
	/*
	{
		VkImageMemoryBarrier image_barrier {};
		image_barrier.sType						= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_barrier.image						= _swapchain_images[ _current_swapchain_image ];
		image_barrier.oldLayout					= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_barrier.newLayout					= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		image_barrier.srcAccessMask				= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		image_barrier.dstAccessMask				= VK_ACCESS_MEMORY_READ_BIT;
		image_barrier.srcQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
		image_barrier.dstQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
		image_barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		image_barrier.subresourceRange.layerCount		= 1;
		image_barrier.subresourceRange.levelCount		= 1;
		image_barrier.subresourceRange.baseArrayLayer	= 0;
		image_barrier.subresourceRange.baseMipLevel		= 0;

		vkCmdPipelineBarrier(
			_render_command_buffers[ _current_swapchain_image ],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier );
	}
	*/
	vkEndCommandBuffer( _render_command_buffers[ _current_swapchain_image ] );

	VkPipelineStageFlags flags[] { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSubmitInfo submit_info {};
	submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount	= 1;
	submit_info.pCommandBuffers		= &_render_command_buffers[ _current_swapchain_image ];
	submit_info.waitSemaphoreCount	= 1;
	submit_info.pWaitSemaphores		= &_present_image_available;
	submit_info.pWaitDstStageMask	= flags;
	vkQueueSubmit( _queue, 1, &submit_info, VK_NULL_HANDLE );

	vkQueueWaitIdle( _queue );

	for( uint32_t i=0; i < _swapchain_image_count; ++i ) {
		vkDestroySemaphore( _device, _render_complete[ i ], nullptr );
	}
	vkDestroySemaphore( _device, _present_image_available, nullptr );
	vkDestroyCommandPool( _device, _command_pool, nullptr );
	_command_pool = VK_NULL_HANDLE;
}

void Window::_UpdateOSWindow()
{
#if VK_USE_PLATFORM_WIN32_KHR
	MSG msg;
	if( PeekMessage( &msg, _win32_window, 0, 0, PM_REMOVE ) ) {
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
#elif VK_USE_PLATFORM_XCB_KHR

#endif
}

void Window::_CreatePipelines()
{
	/*
	// filesystem code. This is C++17 planned feature
	namespace filesys = std::tr2::sys;

	// collect a list of folders inside pipelines folder
	std::vector<filesys::path>		pipeline_folders;
	pipeline_folders.reserve( 128 );
	filesys::directory_iterator		dir_iterator( BUILD_PIPELINE_DIRECTORY );
	for( auto &dir : dir_iterator ) {
		if( filesys::is_directory( dir ) ) {
			pipeline_folders.push_back( dir.path() );
		}
	}
	_pipelines.reserve( pipeline_folders.size() );
	for( auto &path : pipeline_folders ) {
		auto pipe = new Pipeline( _renderer, this, path.string(), path.filename().string() );
		_pipelines.push_back( pipe );
	}
	*/

	auto pipeline_names = _renderer->GetPipelineNames();
	for( auto &n : pipeline_names ) {
		auto pipe = new Pipeline( _renderer, this, n );
		_pipelines.push_back( pipe );
	}
}

void Window::_DestroyPipelines()
{
	for( auto pipe : _pipelines ) {
		delete pipe;
	}
	_pipelines.clear();
}

