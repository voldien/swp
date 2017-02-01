/**
	simple wallpaper program.
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
extern const char* vertex;				/*	Default vertex glsl shader.	*/
extern const char* fragment;			/*	Default fragment glsl shader.	*/
extern const float quad[4][3];			/*	Display quad vertices.	*/


/*	global.	*/
extern int g_alive;						/*	Determine if application should continue be alive or not.*/
extern unsigned int g_compression;		/*	Use compression.	*/
extern unsigned int g_wallpaper;		/*	Wallpaper mode.	*/
extern unsigned int g_fullscreen;		/*	Fullscreen mode.	*/
extern unsigned int g_borderless;		/*	Borderless mode.	*/
extern char* g_fifopath;				/*	Default FIFO filepath.	*/
extern unsigned int g_verbose;			/*	Verbose mode.	*/
extern unsigned int g_debug;			/*	*/
extern FILE* g_verbosefd;				/*	Verbose file descriptor.	*/
extern int g_winres[2];					/*	Window resolution.	*/
extern int g_winpos[2];					/*	Window position.	*/
extern int g_maxtexsize;				/*	OpenGL max texture size, (Check texture proxy later)*/

#define SWP_NUM_TEXTURES 3

typedef struct swp_transition_shader_t{
	GLuint prog;				/*	*/
	float elapse;				/*	*/
	GLint texloc0;				/*	*/
	GLint texloc1;				/*	*/
}swpTransitionShader;

/**
 *
 */
typedef struct swp_rendering_s_t{
	GLuint vao;						/*	*/
	GLuint vbo;						/*	*/
	GLint prog;						/*	Shader program.	*/

	unsigned int numshaders;		/*	*/
	swpTransitionShader* shaders;	/*	*/
	swpTransitionShader* displayshader;

	GLint numtexs;					/*	number of textures in buffer.	*/
	GLuint texs[SWP_NUM_TEXTURES];	/*	texture*/
	GLint curtex;					/*	Current texture displayed.	*/
	GLuint pbo[SWP_NUM_TEXTURES];	/*	Pixel buffer object.*/

	GLint texloc;					/*	*/
	GLint loc;						/*	*/
}swpRenderingData;

/**
 *
 */
typedef struct swp_rendering_state_t{
	unsigned int inTransition;			/*	*/
	unsigned int fromTexIndex;			/*	*/
	unsigned int toTexIndex;			/*	*/
	float elapseTransition;				/*	*/
	swpRenderingData data;				/*	*/
}swpRenderingState;

/**
 *
 */
typedef struct swp_texture_desc_t{
	unsigned int width;		/*	Texture width.	*/
	unsigned int height;	/*	Texture height.	*/
	unsigned int bpp;		/*	Texture Bpp(Byte per pixel).		*/
	unsigned int size;		/*	Size in bytes.	*/
	GLuint intfor;			/*	Texture internal format.	*/
	GLuint format;			/*	Texture input format.*/
	GLuint imgdatatype;		/*	Texture input data type.	*/
	void* pixel;			/*	Remark : free it.	*/
}swpTextureDesc;


/**
 *	Verbose stdout print.
 *
 *	@Return
 */
extern int swpVerbosePrintf(const char* format,...);

/**
 *	Get version of the application.
 *	@Return version non null string.
 */
extern const char* swpGetVersion(void);

/*
 *	Enable debugging.
 */
extern void swpEnableDebug(void);

/**
 *	Parse.
 *	TODO rename to a more generic name.
 */
extern void swpParseResolutionArgument(const char* arg, int* res);


/**
 *	@Return number of bytes that was loaded from file.
 */
extern long int swpLoadFile(const char* cfilename, void** data);


/**
 *
 */
extern void swpGenerateQuad(GLuint* vao, GLuint* vbo);

/**
 *	Load shader.
 *
 *	@Return
 */
extern GLuint swpCreateShader(const char* vshader, const char* fshader);



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
 *	Load texture.
 *
 *	@Return
 */
extern int swpLoadTextureFromMem(GLuint* tex, GLuint pbo, const swpTextureDesc* desc);

/**
 *
 */
extern void swpSetWallpaper(SDL_Window* window);

/**
 *
 */
extern void swpSetFullscreen(SDL_Window* window, Uint32 fullscreen);


/**
 *	@Return
 */
extern void* swpCatchPipedTexture(void* phandle);

/**
 *	Catch software interrupt signals.
 */
extern void swpCatchSignal(int sig);

/**
 *
 */
void swpRender(GLuint vao, SDL_Window* window, swpRenderingState* state);



#endif
