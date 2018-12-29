/**
    Simple wallpaper application.
    Copyright (C) 2016  Valdemar Lindberg

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include <fcntl.h>
#include <FreeImage.h>
#include <getopt.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_video.h>
#include <unistd.h>

#include "wallpaper.h"

static const char *minRequiredExtensions[] = {
		/*  Shaders.    */
		"GL_ARB_fragment_shader",
		"GL_ARB_vertex_shader",
		"GL_ARB_shader_objects",
		/*  Shader features.    */
		"GL_ARB_explicit_attrib_location",

		/*  Buffer objects. */
		"GL_ARB_vertex_buffer_object",
		"GL_ARB_pixel_buffer_object",

		"GL_ARB_multitexture",
		/*  Textures.   */

		/*  */
		"GL_ARB_vertex_array_object",
};
const unsigned int numMinReqExtensions = sizeof(minRequiredExtensions) / sizeof(minRequiredExtensions[0]);


int main(int argc, char** argv){

	int status = EXIT_SUCCESS;      /*	*/
	int result;                     /*	*/
	int i;                          /*	*/
	int x;                          /*	*/
	int fd = 0;                     /*	*/
	int pipe = 0;                   /*	*/
	int fdfifo = 0;                 /*	*/
	int64_t before;

	/*	*/
	int numtranspaths = 0;
	char** transfilepaths = NULL;

	/*	*/
	int visible = 1;

	/*	*/
	swpTextureDesc desc = { 0 };
	SDL_Event event = {0};          /*	*/
	SDL_Thread* thread = NULL;      /*	*/
	swpRenderingState state = {0};  /*	*/
	SDL_Window* window = NULL;      /*	*/
	SDL_GLContext* context = NULL;  /*	*/
	int glatt;                      /*	Tmp value.	*/

	const char* ctitle = "wallpaper";

	/*	OpenGL display buffer.	*/
	GLuint vao;
	GLuint vbo;

	/*	*/
	int c;
	int index;
	const char* shortopt = "vVdf:p:CwFbR:P:s:";
	static struct option longoption[] = {
		{"version",     no_argument, 		NULL, 'v'},	/*	Version of the application.	*/
		{"verbose",     no_argument, 		NULL, 'V'},	/*	Enable verbose.	*/
		{"debug",       no_argument,		NULL, 'd'},	/*	Enable internal debug feature.	*/
		{"wallpaper",   no_argument,	 	NULL, 'w'},	/*	Set display as wallpaper.	*/
		{"fullscreen",  no_argument, 		NULL, 'F'},	/*	Set as fullscreen.	*/
		{"borderless",  no_argument, 		NULL, 'b'},	/*	Set window border less.*/
		{"compression", optional_argument, 	NULL, 'C'},	/*	Enable compression or texture.	*/
		{"fifo",        required_argument, 	NULL, 'p'},	/*	Path for the FIFO.	*/
		{"resolution",  required_argument, 	NULL, 'R'},	/*	Set window resolution.	*/
		{"position",    required_argument, 	NULL, 'P'},	/*	Set window position.	*/
		{"shader",      required_argument, 	NULL, 's'},	/*	*/
		{"socket",      required_argument,	NULL, 'S'},	/*	Listen on socket.	*/
		{"file",        required_argument,	NULL, 'f'},	/*	File to load picture from.	*/
		{"filter",      required_argument,	NULL, 'B'},	/*	Filter.	*/
		{"title",       required_argument,	NULL, 'T'},	/*	Override the title.	*/

		{"row",         required_argument, 	NULL, 'r'},
		{"column",      required_argument, 	NULL, 'c'},
		{NULL, 0, NULL, 0},
	};

	while( ( c = getopt_long(argc, argv, shortopt, longoption, &index) ) != EOF){
		switch(c){
		case 'v':
			printf("version %s\n", swpGetVersion());
			return EXIT_SUCCESS;
		case 'V':
			g_verbose = 1;
			swpVerbosePrintf("Enable verbose.\n");
			break;
		case 'd':
			g_debug = 1;
			swpVerbosePrintf("Enabled debug.\n");
			break;
		case 'p':
			if(optarg){
				g_fifopath = optarg;
			}
			break;
		case 'F':
			g_fullscreen = 1;
			break;
		case 'C':
			g_compression = 1;
			break;
		case 'R':
			if(optarg){
				swpParseResolutionArgument(optarg, &g_winres[0]);
				swpVerbosePrintf("Window size argument to %dx%d \n", g_winres[0], g_winres[1]);
			}
			break;
		case 'P':
			if(optarg){
				swpParseResolutionArgument(optarg, &g_winpos[0]);
				swpVerbosePrintf("Window position argument to %dx%d \n", g_winpos[0], g_winpos[1]);
			}
			break;
		case 'w':
			g_wallpaper = 1;
			break;
		case 'b':
			g_borderless = 1;
			break;
		case 's':
			if(optarg){
				transfilepaths = realloc(transfilepaths, sizeof(char*) * ( numtranspaths + 1 ));
				transfilepaths[numtranspaths] = optarg;
				numtranspaths++;
			}
			break;
		case 'B':
			break;
		case 'f':
			if(optarg){
				fd = open(optarg, O_RDONLY | O_NONBLOCK);
			}
			break;
		case 'T':
			if(optarg){
				ctitle = optarg;
			}
			break;
		default:
			break;
		}
	}

	/*	Check if stdin is piped.	*/
	if(isatty(STDIN_FILENO) == 0){
		pipe = 1;
	}

	/*	Signal.	*/
	signal(SIGINT, swpCatchSignal);
	signal(SIGTERM, swpCatchSignal);
	signal(SIGSEGV, swpCatchSignal);

	/*	Create FIFO.	*/
	result = unlink(g_fifopath);
	if( result > 0){
		fprintf(stderr, "%s.\n", strerror(errno));
		status = EXIT_FAILURE;
		goto error;
	}
	/*	*/
	fdfifo = mkfifo(g_fifopath, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH | O_WRONLY);
	if( fdfifo != 0){
		fprintf(stderr, "Failed to create FIFO file %s\n", strerror(errno));
		status = EXIT_FAILURE;
		goto error;
	}

	/*	Initialize SDL.	*/
	result = SDL_Init(SDL_INIT_VIDEO |  SDL_INIT_EVENTS);
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
	if(result != 0){
		fprintf(stderr, "Failed to initialize SDL, %s.\n", SDL_GetError());
		status = EXIT_FAILURE;
		goto error;
	}

	/*	*/
	FreeImage_Initialise(0);
	swpVerbosePrintf("FreeImage version %s.\n", FreeImage_GetVersion());

	/*	Set window resolution.	*/
	if( g_winres[0] == -1 &&  g_winres[1] == -1){

		SDL_DisplayMode mode;
		SDL_GetCurrentDisplayMode(0, &mode);
		g_winres[0] = mode.w / 2;
		g_winres[1] = mode.h / 2;
	}

	/*	Set window position.	*/
	if( g_winpos[0] == -1 &&  g_winpos[1] == -1){
		SDL_DisplayMode mode;
		SDL_GetCurrentDisplayMode(0, &mode);
		g_winpos[0] = mode.w / 4;
		g_winpos[1] = mode.h / 4;
	}

	/*	Create window.	*/
	window = SDL_CreateWindow(ctitle,
			g_winpos[0], g_winpos[1],
			g_winres[0], g_winres[1],
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
			( g_borderless ? SDL_WINDOW_BORDERLESS : 0 ) );
	if(window == NULL){
		fprintf(stderr, "Failed to create window, %s.\n", SDL_GetError());
		status = EXIT_FAILURE;
		goto error;
	}
	
	/*	*/
	SDL_ShowWindow(window);

	/*  */
	if(g_wallpaper == 1){
		swpSetWallpaper(window);
	}
	if(g_fullscreen == 1){
		swpSetFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}

	/*	Create OpenGL Context.	*/
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	/*	Check if debug is enabled.	*/
	if(g_debug){
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
		        SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &glatt)
		                | SDL_GL_CONTEXT_DEBUG_FLAG);
	}

	/*  */
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, SDL_TRUE);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE);
	SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, SDL_FALSE);

	/*  Disable multi sampling.  */
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, SDL_FALSE);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

	/*  Set minimum version.    */
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);

	context = SDL_GL_CreateContext(window);
	if(context == NULL){
		fprintf(stderr, "Failed create OpenGL core context, %s.\n", SDL_GetError());
		g_core_profile = 0;
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
		context = SDL_GL_CreateContext(window);
		if( context == NULL){
			fprintf(stderr, "Failed to create OpenGL context current, %s.\n", SDL_GetError());
			status = EXIT_FAILURE;
			goto error;
		}
	}
	if( SDL_GL_MakeCurrent(window, context) < 0 ){
		status = EXIT_FAILURE;
		fprintf(stderr, "Failed to set OpenGL context current, %s.\n", SDL_GetError());
		goto error;
	}

	/*  Check if all required extension is supported.   */
	for (i = 0; i < numMinReqExtensions; i++) {
		if (!swpCheckExtensionSupported(minRequiredExtensions[i])) {
			fprintf(stderr, "%s is not supported\n", minRequiredExtensions[i]);
			goto error;
		}
	}

	/*  */
	glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &g_core_profile);
	g_core_profile = (g_core_profile & GL_CONTEXT_CORE_PROFILE_BIT) != 0;

	/*	Create FIFO thread.	*/
	thread = SDL_CreateThread((SDL_ThreadFunction) swpCatchPipedTexture,
			"catch_pipe", &fdfifo);
	if (thread == NULL) {
		fprintf(stderr, "Failed to create thread, %s.\n", SDL_GetError());
		status = EXIT_FAILURE;
		goto error;
	}

	/*	Set OpenGL state.	*/
	SDL_GL_SetSwapInterval(SDL_TRUE);   /*	Enable vsync.	*/
	glDepthMask(GL_FALSE);              /*	Depth mask isn't needed.	*/
	glDepthFunc(GL_LESS);               /**/
	glEnable(GL_DITHER);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);           /*	Depth isn't needed.	*/
	glDisable(GL_STENCIL_TEST);         /*	Stencil isn't needed.	*/
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);
	glCullFace(GL_FRONT_AND_BACK);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	
	/*	Display OpenGL information.	*/
	swpVerbosePrintf("GL_VENDOR %s.\n", glGetString(GL_VENDOR) );
	swpVerbosePrintf("GL_VERSION %s.\n", glGetString(GL_VERSION) );
	swpVerbosePrintf("GL_RENDERER %s.\n", glGetString(GL_RENDERER));
	swpVerbosePrintf("GL_EXTENSION %s.\n", glGetString(GL_EXTENSIONS) );
	swpVerbosePrintf("GL_SHADING_LANGUAGE_VERSION %s.\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	/*	Check OpenGL limitations.	*/
	glGetIntegerv( GL_MAX_TEXTURE_SIZE , &g_maxtexsize);
	swpVerbosePrintf("Max texture size %d.\n", g_maxtexsize);

	/*	Load OpenGL functions.	*/
	swpLoadGLFunc();

	/*	Enable opengl debug callback if in debug mode.	*/
	if(g_debug)
		swpEnableDebug();


	/*	Create display quad.	*/
	swpGenerateQuad(&vao, &vbo);

	/*	State.	*/
	state.timeout = INT32_MAX;

	/*	Initialize rendering data.	*/
	state.data.numtexs = SWP_NUM_TEXTURES;
	state.data.curtex = 0;
	state.data.numshaders = 1;
	state.data.shaders = realloc(state.data.shaders, state.data.numshaders * sizeof(swpTransitionShader));

	/*	Create default shader.	*/
	state.data.displayshader = &state.data.shaders[0];
	state.data.displayshader->prog = swpCreateShader(gc_vertex, gc_fragment);
	if(state.data.displayshader->prog < 0)
		goto error;
	state.data.displayshader->elapse = 0;
	state.data.displayshader->texloc0 = glGetUniformLocationARB(state.data.displayshader->prog, "tex0");
	glUseProgram(state.data.displayshader->prog);
	glUniform1i(state.data.displayshader->texloc0, 0);

	/*	Load transition from file.	 */
	if(numtranspaths > 0)
		swpLoadTransitionShaders(&state, numtranspaths, (const char**)transfilepaths);
	else
		swpCreateDefaultTransitionShader(&state);

	/*	Check if PBO is supported.	*/
	g_support_pbo = swpCheckExtensionSupported("GL_ARB_pixel_buffer_object");

	/*	Create Pixel buffer object.	*/
	if(g_support_pbo)
		glGenBuffersARB(state.data.numtexs, &state.data.pbo[0]);

	/*	Load texture from file.	*/
	if(fd > 0 ){
		swpTextureDesc desc = { 0 };
		swpReadPicFromfd(fd, &desc);
		swpLoadTextureFromMem(&state.data.texs[state.data.curtex],
				state.data.pbo[state.data.curtex], &desc);
		close(fd);
	}

	/*	Load texture from STDIN if piped.	*/
	if(pipe == 1){

		swpReadPicFromfd(STDIN_FILENO, &desc);

		event.user.code = 0;
		event.type = SDL_USEREVENT;
		event.user.data1 = (void*)&desc;
		SDL_PushEvent(&event);
	}

	/*	Initialize texture binding.	*/
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, state.data.texs[state.data.curtex]);

	/*	*/
	while(g_alive != 0){

		/*	Wait intill incoming event.	*/
		while(SDL_WaitEventTimeout(&event, state.timeout)){

			switch(event.type){
			case SDL_APP_TERMINATING:
			case SDL_QUIT:
				swpVerbosePrintf("Requested to quit.");
				goto error;
				break;
			case SDL_KEYUP:
				if(event.key.keysym.sym == SDLK_RETURN && ( event.key.keysym.mod & SDLK_LCTRL ) ){
					swpVerbosePrintf("Set to fullscreen mode.");
					g_fullscreen = ~g_fullscreen & 0x1;
					swpSetFullscreen(window, g_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
				}
				break;
			case SDL_WINDOWEVENT:
				switch(event.window.event){
				case SDL_WINDOWEVENT_CLOSE:
					swpVerbosePrintf("Requested to close Window.");
					g_alive = SDL_FALSE;
					goto error;
					break;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				case SDL_WINDOWEVENT_RESIZED:
					visible = 1;
					glViewport(0, 0, event.window.data1, event.window.data2);
					swpVerbosePrintf("%dx%d\n", event.window.data1, event.window.data2);

					/*	call draw.	*/
					if(visible){
						swpRender(vao, window, &state);
					}

					break;
				case SDL_WINDOWEVENT_HIDDEN:
					visible = 0;
					break;
				case SDL_WINDOWEVENT_EXPOSED:
				case SDL_WINDOWEVENT_SHOWN:
					visible = 1;
					if(visible){
						swpRender(vao, window, &state);
					}
					break;
				}
				break;
			case SDL_RENDER_TARGETS_RESET:

				break;
			case SDL_FINGERMOTION:
				event.tfinger.x;
				break;
			case SDL_SYSWMEVENT:

				break;
			case SDL_USEREVENT:

				/*	Event for when the picture has been loaded from file to memory.	*/
				if(event.user.code == SWP_EVENT_UPDATE_IMAGE){

					/*	*/
					swpLoadTextureFromMem(&state.data.texs[state.data.curtex],
							state.data.pbo[state.data.curtex],
							(swpTextureDesc*)event.user.data1);
					state.data.curtex = (state.data.curtex + 1) % state.data.numtexs;
					glFinish();

					/*	Set transition state.	*/
					if(state.data.numshaders > 1 ){
						state.elapseTransition = 0.0f;
						state.inTransition = 1;

						/*	*/
						state.fromTexIndex = state.data.texs[((state.data.curtex - 2) + SWP_NUM_TEXTURES) % state.data.numtexs];
						state.toTexIndex = state.data.texs[((state.data.curtex - 1) + SWP_NUM_TEXTURES) % state.data.numtexs];

						/*	*/
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, state.toTexIndex);

						/*	*/
						glActiveTexture(GL_TEXTURE1);
						glBindTexture(GL_TEXTURE_2D, state.fromTexIndex);

						/*	Init timer for transition shader.	*/
						before = SDL_GetPerformanceCounter();

					}else if(event.user.code == SWP_EVENT_UPDATE_TRANSITION){

						/*	*/
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, state.data.texs[state.data.curtex]);
					}

					/*	Update window.	*/
					if(visible){
						swpRender(vao, window, &state);
					}
				}else{
					/*	Update the elapse transition time in seconds.	*/
					state.elapseTransition += (float)(SDL_GetPerformanceCounter() - (float)before) / (float)SDL_GetPerformanceFrequency();

					/*	Update window.	*/
					if(visible){
						swpRender(vao, window, &state);
					}
				}

				break;
			default:
				break;
			}
		}
	}

	error:

	/*	Cleanup code.	*/
	g_alive = 0;
	SDL_DetachThread(thread);

	/*	Release OpenGL resources.	*/
	if(context != NULL){
		if(glIsVertexArray(vao) == GL_TRUE){
			glDeleteVertexArrays(1, &vao);
		}
		if(glIsBufferARB(vbo) == GL_TRUE){
			glDeleteBuffersARB(1, &vbo);
		}

		for(i = 0; i < state.data.numtexs; i++){
			if(glIsTexture(state.data.texs[i]) == GL_TRUE){
				glDeleteTextures(1, &state.data.texs[i]);
			}

			if(glIsBufferARB(state.data.pbo[i]) == GL_TRUE){
				glDeleteBuffersARB(1, &state.data.pbo[i]);
			}
		}

		/*	Release Context.	*/
		SDL_GL_MakeCurrent(window, NULL);
		SDL_GL_DeleteContext(context);
	}

	/*	Destroy image.	*/
	if(window != NULL){
		SDL_DestroyWindow(window);
	}

	/*	Release FreeImage and SDL library resources.*/
	FreeImage_DeInitialise();
	SDL_Quit();

	/*	*/
	close(fdfifo);
	close(fd);
	if(g_verbosefd != NULL){
		fclose(g_verbosefd);
	}
	unlink(g_fifopath);

	return status;
}
