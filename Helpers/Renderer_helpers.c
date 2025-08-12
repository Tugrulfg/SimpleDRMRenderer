#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm/drm_fourcc.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <sys/time.h>
#include <unistd.h>
#include "Renderer_helpers.h"

static struct internal_device{
    unsigned int width, height;

    // DRM
    int fd;
    drmModeRes *resources;
    drmModeConnector *connector;
    unsigned int connector_id;
    drmModeCrtc *crtc;
    drmModeModeInfo mode;
    drmModeEncoder *encoder;

    // GBM
    struct gbm_device *gbm;
    struct gbm_surface *gbm_surface;

    // EGL
    EGLDisplay egl_display;
    EGLContext context;
    EGLSurface egl_surface;

    struct gbm_bo *previous_bo;
    uint32_t previous_fb;

    // User defined init and draw functions
    func_t init;
    func_t draw;
    func_t clean;
};

static struct internal_device *dev = NULL;

extern int process_inputs();
unsigned int renderer_get_width(){
	if(!dev){
		printf("Renderer Error: Renderer haven't been initialized\n");
		return 0;
	}

	return dev->width;
}

unsigned int renderer_get_height(){
	if (!dev) {
		printf("Renderer Error: Renderer haven't been initialized\n");
		return 0;
	}

	return dev->height;
}

static int init_drm(){
	const char* cards[] = {"/dev/dri/card0", "/dev/dri/card1"};

	for(unsigned long int i=0; i<sizeof(cards)/sizeof(cards[0]); i++){
		// Open the DRM device
		dev->fd = open(cards[i], O_RDWR | O_CLOEXEC);
		if (dev->fd <= 0) {
			continue;
		}

		printf("Selected card: %s\n", cards[i]);
	}

	if(dev->fd < 0){
		printf("DRM Error: Couldn't find any card available\n");
		return 1;
	}

    // Get DRM resources
    dev->resources = drmModeGetResources(dev->fd);
    if (!dev->resources) {
        printf("DRM Error: Failed to get DRM resources\n");
        close(dev->fd);
        return 1;
    }

    // Select the first connected connector
    dev->connector = NULL;
    dev->crtc = NULL;
    dev->connector_id = -1;
    for (int i = 0; i < dev->resources->count_connectors; i++) {
        dev->connector = drmModeGetConnector(dev->fd, dev->resources->connectors[i]);
        if (dev->connector && dev->connector->connection == DRM_MODE_CONNECTED) {
            dev->connector_id = dev->connector->connector_id;
            break;
        }
        drmModeFreeConnector(dev->connector);
    }
    if (!dev->connector_id) {
        printf("DRM Error: No connected connector found\n");
        drmModeFreeResources(dev->resources);
        close(dev->fd);
        return 1;
    }

    // Get the first valid mode
    dev->mode = dev->connector->modes[0];
    printf("Selected resolution: %dx%d\n", dev->mode.hdisplay, dev->mode.vdisplay);
    dev->width = dev->mode.hdisplay;
    dev->height = dev->mode.vdisplay;

    dev->encoder = drmModeGetEncoder(dev->fd, dev->connector->encoder_id);
    if (!dev->encoder) {
        printf("DRM Error: Failed to get encoder\n");
        drmModeFreeConnector(dev->connector);
        drmModeFreeResources(dev->resources);
        close(dev->fd);
        return 1;
    }

    dev->crtc = drmModeGetCrtc(dev->fd, dev->encoder->crtc_id);

    return 0;
}

static void free_drm(){
	drmModeFreeConnector(dev->connector);
    drmModeFreeEncoder(dev->encoder);
	drmModeFreeResources(dev->resources);
	close(dev->fd);
}

static int init_gbm(){
    // Create a GBM device
    dev->gbm = gbm_create_device(dev->fd);
    if (!dev->gbm) {
        perror("GBM Error: Failed to create GBM device\n");
        return 1;
    }

    // Create a GBM surface
    dev->gbm_surface = gbm_surface_create(dev->gbm, dev->width, dev->height,
                                                     GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!dev->gbm_surface) {
        printf("GBM Error: Failed to create GBM surface\n");
        gbm_device_destroy(dev->gbm);
        return 1;
    }

    return 0;
}

static void free_gbm(){
	gbm_surface_destroy(dev->gbm_surface);
	gbm_device_destroy(dev->gbm);
}

static int init_egl() {
    // 1. Get EGL display
    dev->egl_display = eglGetDisplay(dev->gbm);
    if (dev->egl_display == EGL_NO_DISPLAY) {
        printf("EGL Error: Failed to get EGL Display\n");
        return 1;
    }

    // 2. Bind OpenGL ES API
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        printf("EGL Error: Failed to bind API EGL_OPENGL_ES_API (0x%x)\n", eglGetError());
        return 1;
    }

    // 3. Initialize EGL
    if (!eglInitialize(dev->egl_display, NULL, NULL)) {
        printf("EGL Error: Failed to initialize EGL (0x%x)\n", eglGetError());
        return 1;
    }

    // 4. Choose EGL config
    EGLConfig config;
    EGLint num_configs;
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_SAMPLE_BUFFERS, 1,
        EGL_SAMPLES, 4,
        EGL_NONE
    };

    if (!eglChooseConfig(dev->egl_display, attribs, &config, 1, &num_configs) || num_configs < 1) {
        printf("EGL Error: No suitable EGL config found (0x%x)\n", eglGetError());
        return 1;
    }

    // 5. Create EGL context
    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2, // GLES 2.0
        EGL_NONE
    };
    dev->context = eglCreateContext(dev->egl_display, config, EGL_NO_CONTEXT, contextAttribs);
    if (dev->context == EGL_NO_CONTEXT) {
        printf("EGL Error: Failed to create context (0x%x)\n", eglGetError());
        return 1;
    }

    // 6. Create EGL surface
    dev->egl_surface = eglCreateWindowSurface(dev->egl_display, config, (EGLNativeWindowType)dev->gbm_surface, NULL);
    if (dev->egl_surface == EGL_NO_SURFACE) {
        printf("EGL Error: Failed to create window surface (0x%x)\n", eglGetError());
        eglDestroyContext(dev->egl_display, dev->context);
        return 1;
    }

    // 7. Make context current
    if (!eglMakeCurrent(dev->egl_display, dev->egl_surface, dev->egl_surface, dev->context)) {
        printf("EGL Error: Failed to make context current (0x%x)\n", eglGetError());
        eglDestroySurface(dev->egl_display, dev->egl_surface);
        eglDestroyContext(dev->egl_display, dev->context);
        return 1;
    }

    // 8. Print GL version
    const GLubyte *version = glGetString(GL_VERSION);
    if (!version) {
        printf("GL Error: Failed to query GL version (0x%x)\n", glGetError());
        return 1;
    }
    printf("OpenGL ES Version: %s\n", version);

    return 0;
}

static void free_egl(){
	eglDestroySurface(dev->egl_display, dev->egl_surface);
	eglDestroyContext(dev->egl_display, dev->context);
	eglTerminate(dev->egl_display);
}

static int init_crtc(){
	eglSwapBuffers(dev->egl_display, dev->egl_surface);

	dev->previous_bo = gbm_surface_lock_front_buffer(dev->gbm_surface);

	uint32_t handles[4] = { gbm_bo_get_handle(dev->previous_bo).u32 };
	uint32_t strides[4] = { gbm_bo_get_stride(dev->previous_bo) };
	uint32_t offsets[4] = { 0 };

	if (drmModeAddFB2(dev->fd, dev->width, dev->height, GBM_FORMAT_XRGB8888,
			handles, strides, offsets, &dev->previous_fb, 0)) {
		printf("DRM Error: Failed to create framebuffer\n");
		return 1;
	}

	// Set initial CRTC (only once!)
	if (drmModeSetCrtc(dev->fd, dev->crtc->crtc_id, dev->previous_fb, 0, 0,
			&dev->connector_id, 1, &dev->mode)) {
		printf("DRM Error: Failed to set CRTC\n");
		return 1;
	}

	return 0;
}

static int first_frame = 1;
static int swap_buffers() {
	glFinish();
	eglSwapBuffers(dev->egl_display, dev->egl_surface);

	struct gbm_bo *bo = gbm_surface_lock_front_buffer(dev->gbm_surface);
	if(!bo){
		printf("GBM Error: Failed to lock front buffer\n");
		return 1;
	}

	uint32_t fb;
	uint32_t handles[4] = { gbm_bo_get_handle(bo).u32 };
	uint32_t strides[4] = { gbm_bo_get_stride(bo) };
	uint32_t offsets[4] = { 0 };

	if (drmModeAddFB2(dev->fd, dev->width, dev->height, GBM_FORMAT_XRGB8888,
			handles, strides, offsets, &fb, 0)) {
		gbm_surface_release_buffer(dev->gbm_surface, bo);
		printf("DRM Error: Failed to create framebuffer\n");
		return 1;
	}

	if (first_frame) {
		first_frame = 0;
	}
	else {
		char buf[256];
		read(dev->fd, buf, sizeof(buf));
	}

	// Page Flipping
	if (drmModePageFlip(dev->fd, dev->crtc->crtc_id, fb, DRM_MODE_PAGE_FLIP_EVENT, NULL)) {
		drmModeRmFB(dev->fd, fb);
		printf("DRM Error: Failed to page flip\n");
		return 1;
	}

	drmModeRmFB(dev->fd, dev->previous_fb);
	gbm_surface_release_buffer(dev->gbm_surface, dev->previous_bo);
	dev->previous_bo = bo;
	dev->previous_fb = fb;

	return 0;
}

static void update_fps() {
    // For fps calculation
    static unsigned int frame_count = 0;
    static float fps = 0.0f;
    static struct timeval last_time = {0};

    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    // Calculate the time difference in seconds
    float time_diff = (current_time.tv_sec - last_time.tv_sec) +
                      (current_time.tv_usec - last_time.tv_usec) / 1000000.0f;

    // Increment frame count
    frame_count++;

    // If 1 second has passed, update the FPS
    if (time_diff >= 1.0f) {
        fps = frame_count / time_diff;
        if(fps > 0.000001)					// Skip first frame calculation which results 0 FPS
        	printf("FPS: %.2f\n", fps);

        // Reset for the next second
        frame_count = 0;
        last_time = current_time;
    }
}

int init_renderer(func_t init_f, func_t draw_f, func_t clean_f){
	if(dev){
		printf("Renderer Error: Renderer have already initialized\n");
		return 1;
	}
	else if(!init_f){
		printf("Renderer Error: Invalid init function\n");
		return 1;
	}
	else if (!draw_f) {
		printf("Renderer Error: Invalid draw function\n");
		return 1;
	}
	int ret = 0;


	dev = malloc(sizeof(*dev));
	if(!dev){
		printf("Renderer Error: Malloc failed\n");
		return 1;
	}

	ret = init_drm();
	if(ret){
		free(dev);
		dev = NULL;
		return ret;
	}

	ret = init_gbm();
	if (ret) {
		free_drm();
		free(dev);
		dev = NULL;
		return ret;
	}

	ret = init_egl();
	if (ret) {
		free_gbm();
		free_drm();
		free(dev);
		dev = NULL;
		return ret;
	}

	dev->init = init_f;
	dev->draw = draw_f;
	dev->clean = clean_f;

	printf("Renderer Initialized\n\n");
	return 0;
}

int render_loop(){
	if(!dev){
		printf("Renderer Error: Renderer haven't been initialized\n");
		return 1;
	}
	dev->init();

	if(init_crtc()){
		return 1;
	}

	printf("Render Loop\n------------------------------------------------------------------------\n");
	while(1){
		if(process_inputs())
			break;
		dev->draw();
		if(swap_buffers()){
			return 1;
		}
		update_fps();
	}

	return 0;
}

void free_renderer(){
	if(!dev){
		printf("Renderer Error: Renderer haven't been initialized\n");
		return;
	}

	if(dev->previous_fb)
		drmModeRmFB(dev->fd, dev->previous_fb);

	if(dev->previous_bo)
		gbm_surface_release_buffer(dev->gbm_surface, dev->previous_bo);

	if(dev->clean)
		dev->clean();
	free_egl();
	free_gbm();
	free_drm();
	free(dev);
	dev = NULL;
}
