/**
    Simple wallpaper program.
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
#ifndef _SWP_WALLPAPER_H_
#define _SWP_WALLPAPER_H_ 1
#include <GL/gl.h>
#include <stdio.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_video.h>


/*	Read only.	*/
extern const char* gc_vertex;			/*	Default vertex glsl shader.	*/
extern const char* gc_fragment;			/*	Default fragment glsl shader.	*/
extern const char* gc_fade_transition_fragment;
extern const float gc_quad[4][3];		/*	Display quad vertices.	*/


/*	Global.	*/
extern int g_alive;                     /*	Determine if application should continue be alive or not.*/
extern unsigned int g_compression;      /*	Use compression.	*/
extern unsigned int g_wallpaper;        /*	Wallpaper mode.	*/
extern unsigned int g_fullscreen;       /*	Fullscreen mode.	*/
extern unsigned int g_borderless;       /*	Borderless mode.	*/
extern char* g_fifopath;                /*	Default FIFO filepath.	*/
extern unsigned int g_verbose;          /*	Verbose mode.	*/
extern unsigned int g_debug;            /*	*/
extern FILE* g_verbosefd;               /*	Verbose file descriptor.	*/
extern int g_winres[2];                 /*	Window resolution.	*/
extern int g_winpos[2];                 /*	Window position.	*/
extern int g_maxtexsize;                /*	OpenGL max texture size, (Check texture proxy later)*/
extern int g_support_pbo;               /*	Pixel buffer object for fast image transfer.	*/

#define SWP_NUM_TEXTURES 3

/**
 *	Transition shader and associated
 *	information.
 */
typedef struct swp_transition_shader_t{
	GLuint prog;                /*	Shader program unique ID.	*/
	float elapse;               /*	How time the shader will run.	*/
	GLint normalizedurloc;      /*						*/
	GLint texloc0;              /*	Texture location.	*/
	GLint texloc1;              /*	Texture location.	*/
}swpTransitionShader;

/**
 *	Rendering state data block.
 */
typedef struct swp_rendering_s_t{
	GLuint vao;                     /*	Vertex array object. Used for higher version of OpenGL.	*/
	GLuint vbo;                     /*	Vertex buffer object.	*/
	GLint prog;                     /*	Shader program.	*/

	unsigned int numshaders;        /*	*/
	swpTransitionShader* shaders;   /*	*/
	swpTransitionShader* displayshader;

	GLint numtexs;                  /*	Number of textures in buffer.	*/
	GLuint texs[SWP_NUM_TEXTURES];  /*	texture*/
	GLint curtex;                   /*	Current texture displayed.	*/
	GLuint pbo[SWP_NUM_TEXTURES];   /*	Pixel buffer object.*/

	GLint texloc;                   /*	*/
	GLint loc;                      /*	*/
}swpRenderingData;

/**
 *	Rendering state of the program.
 */
typedef struct swp_rendering_state_t{
	unsigned int inTransition;      /*	*/
	unsigned int fromTexIndex;      /*	*/
	unsigned int toTexIndex;        /*	*/
	float elapseTransition;         /*	*/
	swpRenderingData data;          /*	*/
	unsigned int timeout;           /*	*/
}swpRenderingState;

/**
 *	Texture description used for passing the
 *	data fetched from the FIFO thread to the main thread
 *	for which the OpenGL context resides in order to use
 *	the PBO feature for using DMA (Direct memory access)
 *	for passing the image faster.
 */
typedef struct swp_texture_desc_t{
	unsigned int width;     /*	Texture width.	*/
	unsigned int height;    /*	Texture height.	*/
	unsigned int bpp;       /*	Texture Bpp(Byte per pixel).		*/
	unsigned int size;      /*	Size in bytes.	*/
	GLuint intfor;          /*	Texture internal format.	*/
	GLuint format;          /*	Texture input format.*/
	GLuint imgdatatype;     /*	Texture input data type.	*/
	void* pixel;            /*	Remark : free it.	*/
}swpTextureDesc;


/**
 *	Verbose stdout print. Using the
 *	the same format for which the printf
 *	function uses.
 *
 *	@Return Number of bytes written.
 */
extern int swpVerbosePrintf(const char* format,...);

/**
 *	Get version of the application.
 *	@Return version none null string.
 */
extern const char* swpGetVersion(void);

/**
 *	Enable debugging.
 */
extern void swpEnableDebug(void);

/**
 *	Parse.
 *	TODO rename to a more generic name.
 */
extern void swpParseResolutionArgument(const char* arg, int* res);

/**
 *	Load file from file system.
 *	@Return number of bytes that was loaded from file.
 */
extern long int swpLoadFile(const char* __restrict__ cfilename,
		void** __restrict__ data);

/**
 *	Load string from file. The function uses
 *	swpLoadFile and concatenate null terminated character
 *	at the end of the memory block.
 *
 *	@Return number of bytes that was loaded from file.
 */
extern long int swpLoadString(const char* __restrict__ cfilename,
		void** __restrict__ data);

/**
 *	Generate Quad with two triangle in GL_ARRAY_BUFFER mode.
 */
extern void swpGenerateQuad(GLuint* vao, GLuint* vbo);

/**
 *	Get GLSL version in decimal. Used
 *	for creating glsl version declaration
 *	for the current GLSL version of the bounded
 *	context bounding.
 */
extern unsigned int swpGetGLSLVersion(void);

/**
 *	Check if OpenGL extension is supported on current
 *	OpenGL context.
 *
 *	@Return non-zero if supported, zero otherwise.
 */
extern unsigned int swpCheckExtensionSupported(const char* extension);

/**
 *	create shader from vertex and fragment shader.
 *	The shader source code cannot declare version for
 *	which the glsl code. Because the program will insert
 *	the current version. Instead use the macro feature
 *	in glsl and the __VERSION__ macro variable for
 *	which feature to use in the shader.
 *
 *	@Return non-negative if successfully.
 */
extern GLuint swpCreateShader(const char* vshader, const char* fshader);

/**
 *	Load transition shaders from file paths.
 *	It will create transition shader for each
 *	shader source with the swpCreateTransitionShaders
 *	function.
 *
 *	\state Rendering state object.
 *
 *	\count number of sources in sources pointer array.
 *
 *	\filepaths array of file paths.
 *
 */
extern void swpLoadTransitionShaders(swpRenderingState* __restrict__ state,
		unsigned int count, const char** __restrict__ filepaths);

/**
 *	Create transition shader and get uniform
 *	located associated and cache it.
 *
 *	@Return non-zero if successfully.
 */
extern int swpCreateTransitionShaders(swpTransitionShader* __restrict__ trans,
		const char* __restrict__ source);

/**
 *	Get texture format from freeeimage color space data type.
 *
 *	@Return
 */
extern GLuint swpGetGLTextureFormat(unsigned int ffpic);

/**
 *	Load image from file descriptor.
 *
 *	@Return number of bytes loaded.
 */
ssize_t swpReadPicFromfd(int fd, swpTextureDesc* desc);


/**
 *	Load texture from texture description to
 *	OpenGL texture object with help of PBO
 *	(Pixel buffer object) which will increase the
 *	transfer speed between RAM to VRAM.
 *
 *	\tex OpenGL texture unique identifier.
 *
 *	\pbo Pixel buffer object unique identifier.
 *
 *	\desc Descriptor object with all the required information
 *	about the texture in memory.
 *
 *	@Return non-zero if successfully.
 */
extern int swpLoadTextureFromMem(GLuint* __restrict__ tex, GLuint pbo,
		const swpTextureDesc* __restrict__ desc);

/**
 *	Set window as wallpaper. (Not supported yet)
 *	This will make the program renderer
 *	over the desktop window.
 */
extern void swpSetWallpaper(SDL_Window* window);

/**
 *	Set window in fullscreen mode.
 *	The monitor screen size will not be altered.
 */
extern void swpSetFullscreen(SDL_Window* window, Uint32 fullscreen);


/**
 * 	FIFO thread function for waiting for incoming data.
 *
 *	@Return NULL when terminating the function.
 */
extern void* swpCatchPipedTexture(void* phandle);

/**
 *	Catch software interrupt signals.
 */
extern void swpCatchSignal(int sig);

/**
 *	Render display.
 *
 *	\state will decided for the function
 *	for what should be rendered.
 */
void swpRender(GLuint vao, SDL_Window* __restrict__ window,
        swpRenderingState* __restrict__ state);



#endif
