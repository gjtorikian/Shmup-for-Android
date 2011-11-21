
#include "native_app_glue.h"
#include "android_utils.h"

#include <assert.h>
#include <jni.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#include "../src/dEngine.h"
#include "../src/commands.h"
#include "../src/timer.h"
#include "../src/menu.h"

#include "libpng/png.h"

// output mix interfaces
static SLObjectItf outputMixObject = NULL;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

static jobject assetManager;

vec2_t commScale;

void abort_textureLoading_(const char * s, ...)
{
  LOGE("%s", s);
  abort();
}

void SND_InitSoundTrack(char* str)
{
	JNIEnv* env = engine.app->activity->env;

	initAndroidSound(env, str);
}

void Native_UploadScore(uint score)
{
	if (!engine.gameCenterEnabled)
		return;
	
   // if (engine.licenseType == LICENSE_FULL)
	//    [this reportScore:score forCategory:@"HighScore"];
	//else
	 //   [this reportScore:score forCategory:@"shmup.lite.HighScore"];
	
}

int Native_RetrieveListOf(char replayList[10][256])
{
   /* NSFileManager* fileManager = [NSFileManager defaultManager];
	
	NSString* pathToList = [NSString stringWithFormat:@"%s", FS_GameWritableDir()];
	
	NSDirectoryEnumerator *dirEnum = [fileManager enumeratorAtPath:pathToList];
	
	int numFile = 0;
	
	NSLog(@"[Native_RetrieveListOf]:");
	NSString *file;
	while (file = [dirEnum nextObject]) {
//      printf("File extension %s\n",[[file pathExtension] cStringUsingEncoding:NSASCIIStringEncoding] );
		if ([[file pathExtension] isEqualToString: @"io"]) {
			
			strcpy(replayList[numFile],[file cStringUsingEncoding:NSASCIIStringEncoding]);
			printf("    - Listing %d [%s]\n",numFile,replayList[numFile]);
			numFile++;
		}
	}
	
	return numFile; */
}

void Native_UploadFileTo(char path[256])
{
   // NSString *tmpFilename = [NSString stringWithCString:path encoding:NSASCIIStringEncoding];
   // [engineDelegate sendInputsToServer:tmpFilename];
}

struct zip_file* file;

void png_zip_read(png_structp png_ptr, png_bytep data, png_size_t length) 
{
  zip_fread(file, data, length);
}

void loadNativePNG(texture_t* tmpTex)
{
	png_structp     png_ptr; 
	png_infop       info_ptr; 
	unsigned int    width;
	unsigned int    height;
	int             i;

	int             bit_depth;
	int             color_type ;
	png_size_t      rowbytes;
	png_bytep       *row_pointers;
	png_byte header[8];

  char realPath[1024];

  memset(realPath, 0, 1024);  
  strcat(realPath, FS_Gamedir());
  if (tmpTex->path[0] != '/')
	strcat(realPath, "/");
  strcat(realPath, tmpTex->path);
	
  tmpTex->format = TEXTURE_TYPE_UNKNOWN ;

  file = zip_fopen(APKArchive, realPath, 0);
  
  //LOGI("[Android Main] Opening %s", realPath);

	if ( !file  )
	abort_textureLoading_("Could not open file '%s'\n",tmpTex->path);

	zip_fread(file, header, 8);
	if (png_sig_cmp(header, 0, 8) != 0 )
		abort_textureLoading_("[read_png_file] File is not recognized as a PNG file.\n", tmpTex->path);
	
	// initialize
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (png_ptr == NULL)
		abort_textureLoading_("[read_png_file] png_create_read_struct failed");
	
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
		abort_textureLoading_("[read_png_file] png_create_info_struct failed");
	
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_textureLoading_("[read_png_file] Error during init_io");
	
	png_set_read_fn(png_ptr, NULL, png_zip_read);
	png_set_sig_bytes(png_ptr, 8);
	
	png_read_info(png_ptr, info_ptr);
	
  //Retrieve metadata and transfer to structure bean tmpTex
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
	
	tmpTex->width = width; 
	tmpTex->height =  height;

	// Set up some transforms. 
	/*if (color_type & PNG_COLOR_MASK_ALPHA) {
		png_set_strip_alpha(png_ptr);
	}*/
	if (bit_depth > 8) {
		png_set_strip_16(png_ptr);
	}
	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png_ptr);
	}
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png_ptr);
	}

	// Update the png info struct.
	png_read_update_info(png_ptr, info_ptr);
	
	// Rowsize in bytes. 
	rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	
  	tmpTex->bpp = rowbytes / width;
	if (tmpTex->bpp == 4)
		tmpTex->format = TEXTURE_GL_RGBA;
	else
		tmpTex->format = TEXTURE_GL_RGB;

	LOGI("DEBUG: For %s, bpp: %i, color_type: %i, bit_depth: %i", realPath, tmpTex->bpp, color_type, bit_depth);
  //Since PNG can only store one image there is only one mipmap, allocated an array of one
  tmpTex->numMipmaps = 1;
  tmpTex->data = malloc(sizeof(uchar*));
	if ((tmpTex->data[0] = (uchar*)malloc(rowbytes * height))==NULL) 
  {
	//Oops texture won't be able to hold the result :(, cleanup LIBPNG internal state and return;
	free(tmpTex->data);
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	return;
	}
	
  //Next we need to send to libpng an array of pointer, let's point to tmpTex->data[0]
	if ((row_pointers = (png_bytepp)malloc(height*sizeof(png_bytep))) == NULL) 
  {
	// Oops looks like we won't have enough RAM to allocate an array of pointer....
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		free(tmpTex->data );
		tmpTex->data  = NULL;
	return;
	}

  //FCS: Hm, it looks like we are flipping the image vertically.
  //     Since iOS did not do it, we may have to not to that. If result is 
  //     messed up, just swap to:   row_pointers[             i] = ....
	for (i = 0;  i < height;  ++i) 
		//row_pointers[height - 1 - i] = tmpTex->data[0]  + i * rowbytes;
	row_pointers[             i] = tmpTex->data[0]  + i*rowbytes;


  //Decompressing PNG to RAW where row_pointers are pointing (tmpTex->data[0])
	png_read_image(png_ptr, row_pointers);
	
  //Last but not least:


	// Free LIBPNG internal state.
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  //Free the decompression buffer
  free(row_pointers);

	zip_fclose(file);
}

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine_info_t* engine) {
	uchar engineParameters = 0;
	char cwd[256];
	char lpBuffer[256];
 
	engineParameters |= GL_11_RENDERER ;
  
	setenv( "RD", "assets", 1 ); 
	setenv( "WD", "mnt/sdcard/app-data/com.miadzin.shmup", 1 );

	assetManager = getAssetManager();
	
	renderer.materialQuality = MATERIAL_QUALITY_HIGH;
	renderer.glBuffersDimensions[WIDTH] = 480;
	renderer.glBuffersDimensions[HEIGHT] = 800;
	renderer.resolution = 1.5f;
	renderer.props |= PROP_FOG;
	renderer.props |= PROP_SHADOW ;
	renderer.props |= PROP_BUMP ;
	renderer.props |= PROP_SPEC ;
	
	int gameOn = 1;

	dEngine_Init();


	// initialize OpenGL ES and EGL

	/*
	 * Here specify the attributes of the desired configuration.
	 * Below, we select an EGLConfig with at least 8 bits per color
	 * component compatible with on-screen windows
	 */
	const EGLint attribs[] = {
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_BLUE_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_RED_SIZE, 8,
			EGL_NONE
	};
	EGLint w, h, dummy, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);

	/* Here, the application chooses the configuration it desires. In this
	 * sample, we have a very simplified selection process, where we pick
	 * the first EGLConfig that matches our criteria */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	 * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	 * As soon as we picked a EGLConfig, we can safely reconfigure the
	 * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w;
	engine->height = h;
	//engine->state.angle = 0;

	// Initialize GL state.
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);

  	dEngine_InitDisplaySystem(engineParameters);

  	commScale[X] = commScale[Y] = renderer.resolution;
  	
	return 0;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine_info_t* engine) {
	if (engine->display == NULL) {
		// No display.
		return;
	}

	dEngine_HostFrame();

	eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine_info_t* engine) {
	if (engine->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT) {
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE) {
			eglDestroySurface(engine->display, engine->surface);
		}
		eglTerminate(engine->display);
	}
	engine->animating = 0;
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
}

int lastTouchBegan = 0;
#define SQUARE(X) ((X)*(X))
/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* androidEngine = (struct engine*)app->userData;

	int touchCount = 0;
	static int previousTouchCount;
	int numButton;
	touch_t* touch;
	touch_t* currentTouchSet;

	struct TOUCHSTATE *touchstate = 0;
	struct TOUCHSTATE *prev_touchstate = 0;

	int nSourceId = AInputEvent_getSource( event );

	if (nSourceId == AINPUT_SOURCE_TOUCHPAD)
   		touchstate = engine.touchstate_pad; // GJT: For Xperia Play...and other devices?
	else if (nSourceId == AINPUT_SOURCE_TOUCHSCREEN)
    	touchstate = engine.touchstate_screen;
    else
    	return 0; // GJT: Volume? Keyboard? Let the system handle it...

	if (engine.menuVisible)
	{
		numButton = MENU_GetNumButtonsTouches();
		currentTouchSet = MENU_GetCurrentButtonTouches();		
	}
	else 
	{
		numButton = NUM_BUTTONS;
		currentTouchSet = touches;
	}

	if (engine.menuVisible || engine.controlMode == CONTROL_MODE_VIRT_PAD)
	{
		size_t pointerCount =  AMotionEvent_getPointerCount(event);
		size_t i;
		for (i = 0; i < pointerCount; i++) 
		{
            size_t pointerId = AMotionEvent_getPointerId(event, i);
	        size_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
		
			touchstate[pointerId].x = AMotionEvent_getX( event, i );
			touchstate[pointerId].y = AMotionEvent_getY( event, i );
			
			LOGI("DEBUG BEFORE x %f y %f", touchstate[pointerId].x, touchstate[pointerId].y);

			// Transforming from whatever screen resolution we have to the original iPhone 320*480
			touchstate[pointerId].x = ( touchstate[pointerId].x - renderer.viewPortDimensions[VP_X] ) * commScale[X] ;
			touchstate[pointerId].y = ( touchstate[pointerId].y - renderer.viewPortDimensions[VP_Y] ) * commScale[Y] ;

			LOGI("DEBUG AFTER  x %f y %f", touchstate[pointerId].x, touchstate[pointerId].y);

			touchCount++;

			// find which one it is closest to
			int		minDist = INT_MAX; // allow up to 64 unit moves to be drags
			int		minIndex = -1;
			int dist;
			touch_t	*t2 = currentTouchSet;
			int i;

			for ( i = 0 ; i < numButton ; i++ ) 
			{
				dist = SQUARE( t2->iphone_coo_SysPos[X] - touchstate[pointerId].x )  + SQUARE( t2->iphone_coo_SysPos[Y] - touchstate[pointerId].y ) ;

				LOGI("DEBUG dist %i minDist %i", dist, minDist);
				if ( dist < minDist ) {
					minDist = dist;
					minIndex = i;
					touch = t2;
				}
				t2++;
			}

			if ( minIndex != -1 ) 
			{
				if (action == AMOTION_EVENT_ACTION_UP) 
				{
					touch->down = 0;
				}
				else 
				{
					if (action == AMOTION_EVENT_ACTION_DOWN) 
					{
					}
					touch->down = 1;
					touch->dist[X] = MIN(1,(touchstate[pointerId].x - touches[minIndex].iphone_coo_SysPos[X])/touches[minIndex].iphone_size);
					touch->dist[Y] = MIN(1,(touches[minIndex].iphone_coo_SysPos[Y] - touchstate[pointerId].y)/touches[minIndex].iphone_size);
					LOGI("DEBUG minIndexIphone %i", touches[minIndex].iphone_size);
				}
			}
		}

		LOGI("DEBUG touchcount %i previous %i", touchCount, previousTouchCount);
		if ( touchCount == 5 && previousTouchCount != 5 ) 
		{
			MENU_Set(MENU_HOME);
			engine.requiredSceneId = 0;
		}
	
		previousTouchCount = touchCount;

		return 1;
	}

	else
	{
		size_t pointerCount =  AMotionEvent_getPointerCount(event);
		size_t i;

		for (i = 0; i < pointerCount; i++) 
		{
			size_t pointerCount =  AMotionEvent_getPointerCount(event);
			size_t pointerId = AMotionEvent_getPointerId(event, i);
	        size_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
	        size_t pointerIndex = i;
		
			int historySize = AMotionEvent_getHistorySize(event);

			touchstate[pointerId].x = AMotionEvent_getX( event, pointerIndex );
			touchstate[pointerId].y = AMotionEvent_getY( event, pointerIndex );

			LOGI("DEBUG history %i", historySize);

			LOGI("DEBUG nomenu x %f y %f", touchstate[pointerId].x, touchstate[pointerId].y);
			if (historySize > 0)
			{
				//prev_touchstate[pointerId].x = AMotionEvent_getX( event, pointerIndex );
				//prev_touchstate[pointerId].y = AMotionEvent_getY( event, pointerIndex );

				LOGI("DEBUG nomenu prevx %f prevy %f", prev_touchstate[pointerId].x, prev_touchstate[pointerId].y);
			}
			//Transforming from whatever screen resolution we have to the original iPHone 320*480
			/*touchstate[pointerId].x = ( touchstate[pointerId].x- renderer.viewPortDimensions[VP_X] ) * commScale[X] ;//* renderer.resolution ;
			touchstate[pointerId].y = ( touchstate[pointerId].y- renderer.viewPortDimensions[VP_Y] ) * commScale[Y] ;//* renderer.resolution;
			
			prev_touchstate[pointerId].x = ( prev_touchstate[pointerId].x- renderer.viewPortDimensions[VP_X] ) * commScale[X] ;//* renderer.resolution ;
			prev_touchstate[pointerId].y = ( prev_touchstate[pointerId].y- renderer.viewPortDimensions[VP_Y] ) * commScale[Y] ;//* renderer.resolution;*/
			
			
			touchCount++;					

			if (action == AMOTION_EVENT_ACTION_UP) 
			{
				if (touchCount == 1) //Last finger ended
					touches[BUTTON_FIRE].down = 0;
			}
			else 
			{
				if (action == AMOTION_EVENT_ACTION_MOVE) 
				{
					//printf("m\n");
								LOGI("DEBUG MOVE x %f y %f", touchstate[pointerId].x, touchstate[pointerId].y);
			//LOGI("DEBUG MOVE prevx %f prevy %f", prev_touchstate[pointerId].x, prev_touchstate[pointerId].y);

					touches[BUTTON_MOVE].down = 1;
					//touches[BUTTON_MOVE].dist[X] = (touchstate[pointerId].x - prev_touchstate[pointerId].x)*40/(float)320;
					//touches[BUTTON_MOVE].dist[Y] = (touchstate[pointerId].y - prev_touchstate[pointerId].y)*-40/(float)480;
					
				}
				if (action == AMOTION_EVENT_ACTION_DOWN)
				{
					int currTime = E_Sys_Milliseconds();
					if (currTime-lastTouchBegan < 200)
						touches[BUTTON_GHOST].down = 1;
					
					lastTouchBegan = currTime ;
					
					touches[BUTTON_FIRE].down = 1;
				}
				
				
			}
		}

		if ( touchCount == 5 && previousTouchCount != 5 ) 
		{
			MENU_Set(MENU_HOME);
			engine.requiredSceneId=0;
		}
	
		previousTouchCount = touchCount;

		return 1;
	}
	return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine_info_t* engine = (struct engine_info_t*)app->userData;
	switch (cmd) {
		case APP_CMD_SAVE_STATE:
			// The system has asked us to save our current state.  Do so.
			//engine->app->savedState = malloc(sizeof(struct saved_state));
			//*((struct saved_state*)engine->app->savedState) = engine->state;
			//engine->app->savedStateSize = sizeof(struct saved_state);
			break;
		case APP_CMD_INIT_WINDOW:
			// The window is being shown, get it ready.
			if (engine->app->window != NULL) {
				engine_init_display(engine);
				engine_draw_frame(engine);
			}
			break;
		case APP_CMD_TERM_WINDOW:
			// The window is being hidden or closed, clean it up.
			engine_term_display(engine);
			break;
		case APP_CMD_GAINED_FOCUS:
			// When our app gains focus, we start monitoring the accelerometer.
			/*if (engine->accelerometerSensor != NULL) {
				ASensorEventQueue_enableSensor(engine->sensorEventQueue,
						engine->accelerometerSensor);
				// We'd like to get 60 events per second (in us).
				ASensorEventQueue_setEventRate(engine->sensorEventQueue,
						engine->accelerometerSensor, (1000L/60)*1000);
			}*/
			break;
		case APP_CMD_LOST_FOCUS:
			// When our app loses focus, we stop monitoring the accelerometer.
			// This is to avoid consuming battery while not being used.
			/*if (engine->accelerometerSensor != NULL) {
				ASensorEventQueue_disableSensor(engine->sensorEventQueue,
						engine->accelerometerSensor);
			}*/
			// Also stop animating.
			engine->animating = 0;
			engine_draw_frame(engine);
			break;
	}
}

int32_t getTickCount() {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec*1000000000LL + now.tv_nsec;
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
	//struct engine engine;

const int FRAMES_PER_SECOND = 60;
const int SKIP_TICKS = 1000 / FRAMES_PER_SECOND;

	// Make sure glue isn't stripped.
	app_dummy();

	memset(&engine, 0, sizeof(engine));
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app = state;
	
	createSoundEngine();
	createBufferQueueAudioPlayer();

	// Prepare to monitor accelerometer
	/*engine.sensorManager = ASensorManager_getInstance();
	engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
			ASENSOR_TYPE_ACCELEROMETER);
	engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
			state->looper, LOOPER_ID_USER, NULL, NULL);*/

	if (state->savedState != NULL) {
		// We are starting with a previous saved state; restore from it.
	   // engine.state = *(struct saved_state*)state->savedState;
	}

	int32_t next_game_tick = getTickCount();
	int sleep_time = 0;
//http://www.koonsolo.com/news/dewitters-gameloop/
	// loop waiting for stuff to do.
	while (1) {
		// Read all pending events.
		int ident;
		int events;
		struct android_poll_source* source;

		// If not animating, we will block forever waiting for events.
		// If animating, we loop until all events are read, then continue
		// to draw the next frame of animation.
		if ((ident = ALooper_pollAll(0, NULL, &events,
				(void**)&source)) >= 0) {
			// Process this event.
			if (source != NULL) {
				source->process(state, source);
			}
		}

		engine_draw_frame(&engine);
		next_game_tick += SKIP_TICKS;
        sleep_time = next_game_tick - now_ms();
        if( sleep_time >= 0 ) {
            sleep( sleep_time );
        }
	}

	shutdownAudio();
}
//END_INCLUDE(all)
