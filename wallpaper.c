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

#include "wallpaper.h"

#include <fcntl.h>
#include <FreeImage.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <assert.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_thread.h>
#include <unistd.h>

/*	*/
#ifdef GLES2
	#undef GLES2
	#include<GLES2/gl2.h>
	#include<GLES2/gl2ext.h>
#else
	#include <GL/gl.h>
	#include <GL/glext.h>
#endif


/*	Default vertex shader.	*/
const char* gc_vertex = ""
"#if __VERSION__ > 130\n"
"layout(location = 0) in vec3 vertex;\n"
"#else\n"
"attribute vec3 vertex;\n"
"#endif\n"
"#if __VERSION__ > 120\n"
"smooth out vec2 uv;\n"
"#else\n"
"varying vec2 uv;\n"
"#endif\n"
"void main(void){\n"
"	gl_Position = vec4(vertex,1.0);\n"
"	uv = (vertex.xy + vec2(1.0)) / 2.0;"
"}\n";

/*	Default fragment shader.	*/
const char* gc_fragment = ""
"#if __VERSION__ > 130\n"
"layout(location = 0) out vec4 fragColor;\n"
"#elif __VERSION__ == 130\n"
"out vec4 fragColor;\n"
"#endif\n"
"uniform sampler2D tex0;\n"
"#if __VERSION__ > 120\n"
"smooth in vec2 uv;\n"
"#else\n"
"varying vec2 uv;\n"
"#endif\n"
"void main(void){\n"
"	#if __VERSION__ > 120\n"
"	fragColor = texture(tex0, uv);\n"
"	#else\n"
"	gl_FragColor = texture2D(tex0, uv);\n"
"	#endif\n"
"}\n";



/*	Display quad.	*/
const float gc_quad[4][3] = {
		{-1.0f, -1.0f, 0.0f},
		{-1.0f,  1.0f, 0.0f},
		{ 1.0f, -1.0f, 0.0f},
		{ 1.0f,  1.0f, 0.0f},
};


/*	global.	*/
int g_alive = 1;						/*	*/
unsigned int g_compression = 0;			/*	Use compression.	*/
unsigned int g_wallpaper = 0;			/*	Wallpaper mode.	*/
unsigned int g_fullscreen = 0;			/*	Fullscreen mode.	*/
unsigned int g_borderless = 0;			/*	Borderless mode.	*/
char* g_fifopath = "/tmp/wallfifo0";	/*	Default FIFO filepath.	*/
unsigned int g_verbose = 0;				/*	Verbose mode.	*/
unsigned int g_debug = 0;				/*	Debug mode.	*/
FILE* g_verbosefd = NULL;				/*	Verbose file descriptor.	*/
int g_winres[2] = {-1,-1};				/*	Window resolution.	*/
int g_winpos[2] = {-1,-1};				/*	Window position.	*/
int g_maxtexsize;
int g_support_pbo = 0;


typedef void (APIENTRY *DEBUGPROC)(GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar *message,
		void *userParam);
typedef void(*glDebugMessageCallback)(DEBUGPROC,void*);


int swpVerbosePrintf(const char* format,...){

	int status = -1;

	if(g_verbose != 0){
		va_list vl;
		va_start(vl, format);
		status = vfprintf(stdout, format, vl);
		va_end(vl);
	}
	return status;
}

const char* swpGetVersion(void){
	return SWP_STR_VERSION;
}

void callback_debug_gl(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam){

	char* stringStream;
    char* sourceString;
    char* typeString;
    char* severityString;

    switch (source) {
        case GL_DEBUG_CATEGORY_API_ERROR_AMD:
        case GL_DEBUG_SOURCE_API: {
            sourceString = "API";
            break;
        }
        case GL_DEBUG_CATEGORY_APPLICATION_AMD:
        case GL_DEBUG_SOURCE_APPLICATION: {
            sourceString = "Application";
            break;
        }
        case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: {
            sourceString = "Window System";
            break;
        }
        case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:
        case GL_DEBUG_SOURCE_SHADER_COMPILER: {
            sourceString = "Shader Compiler";
            break;
        }
        case GL_DEBUG_SOURCE_THIRD_PARTY: {
            sourceString = "Third Party";
            break;
        }
        case GL_DEBUG_CATEGORY_OTHER_AMD:
        case GL_DEBUG_SOURCE_OTHER: {
            sourceString = "Other";
            break;
        }
        default: {
            sourceString = "Unknown";
            break;
        }
    }/**/
    printf(sourceString);

    switch (type) {
        case GL_DEBUG_TYPE_ERROR: {
            typeString = "Error";
            break;
        }
        case GL_DEBUG_CATEGORY_DEPRECATION_AMD:
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: {
            typeString = "Deprecated Behavior";
            break;
        }
        case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: {
            typeString = "Undefined Behavior";
            break;
        }
        case GL_DEBUG_TYPE_PORTABILITY_ARB: {
            typeString = "Portability";
            break;
        }
        case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:
        case GL_DEBUG_TYPE_PERFORMANCE: {
            typeString = "Performance";
            break;
        }
        case GL_DEBUG_CATEGORY_OTHER_AMD:
        case GL_DEBUG_TYPE_OTHER: {
            typeString = "Other";
            break;
        }
        default: {
            typeString = "Unknown";
            break;
        }
    }/**/
    printf(typeString);

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: {
            severityString = "High";
            break;
        }
        case GL_DEBUG_SEVERITY_MEDIUM: {
            severityString = "Medium";
            break;
        }
        case GL_DEBUG_SEVERITY_LOW: {
            severityString = "Low";
            break;
        }
        default: {
            severityString = "Unknown";
            break;
        }
    }/**/
    printf(severityString);

    printf(message);
    printf("\n");
}

void swpEnableDebug(void){
    glDebugMessageCallback __glDebugMessageCallback;
    glDebugMessageCallback __glDebugMessageCallbackARB;
    glDebugMessageCallback __glDebugMessageCallbackAMD;

    /*	Load function pointer by their symbol name.	*/
    __glDebugMessageCallback = (glDebugMessageCallback)SDL_GL_GetProcAddress("glDebugMessageCallback");
    __glDebugMessageCallbackARB  = (glDebugMessageCallback)SDL_GL_GetProcAddress("glDebugMessageCallbackARB");
    __glDebugMessageCallbackAMD  = (glDebugMessageCallback)SDL_GL_GetProcAddress("glDebugMessageCallbackAMD");


    /*	Set Debug message callback.	*/
    if(__glDebugMessageCallback){
        __glDebugMessageCallback(callback_debug_gl, NULL);
    }
    if(__glDebugMessageCallbackARB){
        __glDebugMessageCallbackARB(callback_debug_gl, NULL);
    }
    if(__glDebugMessageCallbackAMD){
        __glDebugMessageCallbackAMD(callback_debug_gl, NULL);
    }

    /*	*/
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}


void swpParseResolutionArgument(const char* arg, int* res){

	char* lws;
	char* rws;
	char optarg[1024];
	memcpy(optarg, arg, strlen(arg) + 1);

	/*	Parse.	*/
	lws = strstr(optarg, "x");
	*lws = '\0';
	rws = (char*)(lws + 1);
	lws = optarg;
	res[0] = (int)strtol((const char *)lws, NULL, 10);
	res[1] = (int)strtol((const char *)rws, NULL, 10);
}


long int swpLoadFile(const char* cfilename, void** data){

	FILE* f;			/*	*/
	long int pos;		/*	*/
	long int nBytes;	/*	*/


	f = fopen(cfilename, "rb");
	if(f == NULL){
		fprintf(stderr, "Failed to open %s, %s.\n", cfilename, strerror(errno) );
		return -1;
	}


	/*	Determine size of file.	*/
	fseek(f, 0, SEEK_END);
	pos = ftell(f);
	fseek(f, 0, SEEK_SET);

	data[0] = malloc(pos);
	assert(data[0]);

	nBytes = fread(data[0], 1, pos, f );

	fclose(f);

	return nBytes;
}

long int swpLoadString(const char* __restrict__ cfilename,
		void** __restrict__ data){

	long int l;

	l = swpLoadFile(cfilename, data);
	if(l > 0){
		*data = realloc(*data, l + 1);
		((char**)*data)[l] = '\0';
	}

	return l;
}

void swpGenerateQuad(GLuint* vao, GLuint* vbo){

	swpVerbosePrintf("Generating display quad.\n");
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);

	glGenBuffers(1, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gc_quad), gc_quad, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, NULL);

	glBindVertexArray(0);
}

unsigned int swpGetGLSLVersion(void){

	unsigned int version;
	char glstring[128] = {0};
	char* wspac;

	/*	Extract version number.	*/
	strcpy(glstring, (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
	wspac = strstr(glstring, " ");
	if(wspac){
		*wspac = '\0';
	}
	version = strtof(glstring, NULL) * 100;

	return version;
}

unsigned int swpCheckExtensionSupported(const char* extension){

	int i;
	GLint k;
	PFNGLGETSTRINGIPROC glGetStringi = NULL;

	/*	*/
	glGetStringi = (PFNGLGETSTRINGIPROC)SDL_GL_GetProcAddress("glGetStringi");

	/*	*/
	if(glGetStringi){

		/*	*/
		glGetIntegerv(GL_NUM_EXTENSIONS, &k);

		/*	Iterate each extensions.	*/
		for(i = 0; i < k; i++){
			const GLubyte* nextension = glGetStringi(GL_EXTENSIONS, i);
			if(nextension){
				if(strstr(nextension, extension))
					return 1;
			}
		}
	}else
		return strstr(glGetString(GL_EXTENSIONS), extension) != NULL;

	return 0;
}


GLuint swpCreateShader(const char* vshader, const char* fshader){

	GLuint vs, fs;
	GLuint prog;
	GLuint vstatus, lstatus;
	const char* vsources[2];
	const char* fsources[2];
	char glversion[32];

	int value;
	const char* strcore;


	swpVerbosePrintf("Loading shader program.\n");

	/*	Check if core.	*/
	glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &value);
	strcore = ( value == SDL_GL_CONTEXT_PROFILE_CORE ) ? "core" : "";

	/*	Create version string.	*/
	sprintf(glversion, "#version %d %s\n", swpGetGLSLVersion(), strcore);	/*	TODO evalute.*/
	vsources[0] = glversion;
	fsources[0] = glversion;


	prog = glCreateProgram();

	if(vshader != NULL){
		vsources[1] = vshader;
		vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vs, 2, vsources, NULL);
		glCompileShader(vs);
		glAttachShader(prog, vs);
	}
	if(fshader != NULL){
		fsources[1] = fshader;
		fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs, 2, fsources, NULL);
		glCompileShader(fs);
		glAttachShader(prog, fs);
	}

	/*	Link shader.	*/
	glLinkProgram(prog);
	glValidateProgram(prog);

	/*	Check status of linking and validate.	*/
	glGetProgramiv(prog, GL_LINK_STATUS, &lstatus);
	glGetProgramiv(prog, GL_VALIDATE_STATUS, &vstatus);


	/*	Check if link successfully.	*/
	if(lstatus == GL_FALSE){
		char info[4096];
		glGetProgramInfoLog(prog, sizeof(info), NULL, &info[0]);
		fprintf(stderr, "%s\n", info);
		return -1;
	}
	if(vstatus == GL_FALSE){
		char info[4096];
		glGetProgramInfoLog(prog, sizeof(info), NULL, &info[0]);
		fprintf(stderr, "%s\n", info);
	}


	glBindAttribLocation(prog, 0,  "vertex");
	glBindFragDataLocation(prog, 0, "fragColor");

	/*	Detach shaderer object and release their resources.	*/
	glDetachShader(prog, vs);
	glDetachShader(prog, fs);
	glDeleteShader(vs);
	glDeleteShader(fs);

	return prog;
}

void swpLoadTransitionShaders(swpRenderingState* state, unsigned int count, char** sources){

	int x;

	swpVerbosePrintf("Loading %d transition shaders.\n", count);

	/*	Iterate through each shader.	*/
	for(x = 0; x < count; x++){
		void* fragdata = NULL;
		int index = state->data.numshaders;
		swpTransitionShader* trans;

		state->data.numshaders++;
		state->data.shaders = realloc(state->data.shaders, state->data.numshaders * sizeof(swpTransitionShader));
		assert(state->data.shaders);
		trans = &state->data.shaders[index];

		/*	*/
		if( swpLoadFile(optarg, &fragdata) > 0){
			trans->prog = swpCreateShader(gc_vertex, fragdata);
			trans->elapse = 1.120f;
			trans->normalizedurloc = glGetUniformLocation(trans->prog, "normalizedur");
		}

	}
}


GLuint swpGetGLTextureFormat(unsigned int ffpic){

	switch(ffpic){
	case FIT_BITMAP:
		return GL_UNSIGNED_BYTE;
	case FIT_UINT16:
		return GL_UNSIGNED_SHORT;
	case FIT_INT16:
		return GL_SHORT;
	case FIT_UINT32:
		return GL_UNSIGNED_INT;
	case FIT_INT32:
		return GL_INT;
	case FIT_FLOAT:
		return GL_FLOAT;
	case FIT_RGB16:
		return GL_RGB16;
	case FIT_RGBA16:
		return GL_RGBA16;
	case FIT_RGBF:
	case FIT_RGBAF:
	default:
		fprintf(stderr, "Coulnd't find OpenGL data type for texture.\n");
		return 0;
	}
}


ssize_t swpReadPicFromfd(int fd, swpTextureDesc* desc){

	/*	*/
	char inbuf[4096];					/**/
	register ssize_t len = 0;			/**/
	register ssize_t totallen = 0;		/**/


	/*	Free image.	*/
	FREE_IMAGE_FORMAT imgtype;			/**/
	FREE_IMAGE_COLOR_TYPE colortype;	/**/
	FREE_IMAGE_TYPE imgt;				/**/
	FIMEMORY *stream;					/**/
	FIBITMAP* firsbitmap;				/**/
	FIBITMAP* bitmap;					/**/
	void* pixel;						/**/

	/*	*/
	unsigned int width;
	unsigned int height;
	unsigned int bpp;
	unsigned int size;

	swpVerbosePrintf("Starting loading image.\n");

	/*	1 byte for the size in order, Because it crash otherwise if set to 0.	*/
	stream = FreeImage_OpenMemory(NULL, 1);
	if(stream == NULL){
		fprintf(stderr, "Failed to open freeimage memory stream. \n");
	}


	/*	Read from file stream.	*/
	while( (len = read(fd, inbuf, sizeof(inbuf)) ) > 0){
		if(len < 0){
			fprintf(stderr, "Error reading image, %s.\n", strerror(errno));
			FreeImage_CloseMemory(stream);
			return 0;
		}

		FreeImage_WriteMemory(inbuf, 1, len, stream);
		totallen += len;
	}


	/*	Seek to beginning of the memory stream.	*/
	FreeImage_SeekMemory(stream, 0, SEEK_SET);
	swpVerbosePrintf("Image file size %ld\n", totallen);


	/*	Load image from */
	imgtype = FreeImage_GetFileTypeFromMemory(stream, totallen);
	FreeImage_SeekMemory(stream, 0, SEEK_SET);
	firsbitmap = FreeImage_LoadFromMemory(imgtype, stream, 0);
	if( firsbitmap == NULL){
		fprintf(stderr, "Failed to create free-image from memory.\n");
		FreeImage_CloseMemory(stream);
		return -1;
	}


	/*	*/
	FreeImage_SeekMemory(stream, 0, SEEK_SET);
	imgt = FreeImage_GetImageType(firsbitmap);
	colortype = FreeImage_GetColorType(firsbitmap);


	/*	Input data type.	*/
	swpVerbosePrintf("image pixel data type %d\n", imgt);
	desc->imgdatatype = swpGetGLTextureFormat(imgt);

	/*	Get texture color type.	*/
	swpVerbosePrintf("image color type %d\n", colortype);
	switch(colortype){
	case FIC_RGB:
		bitmap = FreeImage_ConvertTo32Bits(firsbitmap);
		desc->intfor = GL_RGB;
		desc->format = GL_BGRA;
		break;
	case FIC_RGBALPHA:
		bitmap = FreeImage_ConvertTo32Bits(firsbitmap);
		desc->intfor = GL_RGBA;
		desc->format = GL_BGRA;
		break;
	case FIC_PALETTE:
		bitmap = FreeImage_ConvertTo32Bits(firsbitmap);
		desc->intfor = GL_RGBA;
		desc->format = GL_BGRA;
		break;
	case FIC_CMYK:
		bitmap = FreeImage_ConvertTo32Bits(firsbitmap);
		desc->intfor = GL_RGBA;
		desc->format = GL_BGRA;
		break;
	default:
		fprintf(stderr, "None supported freeimage color type, %d.\n", colortype);
		FreeImage_CloseMemory(stream);
		FreeImage_Unload(firsbitmap);
		return -1;
	}


	/*	Check if the conversion was successfully.	*/
	if(bitmap == NULL){
		fprintf(stderr, "Failed to convert bitmap.\n");
		FreeImage_CloseMemory(stream);
		FreeImage_Unload(firsbitmap);
		return -1;
	}


	/*	Get attributes from the image.	*/
	pixel = FreeImage_GetBits(bitmap);
	width = FreeImage_GetWidth(bitmap);
	height = FreeImage_GetHeight(bitmap);
	bpp = ( FreeImage_GetBPP(bitmap) / 8 );
	size = width * height * bpp;
	swpVerbosePrintf("%d kb, %d %dx%d\n", size / 1024, imgtype, width, height);

	/*	Check size.	*/
	if(width > g_maxtexsize || height > g_maxtexsize){
		fprintf(stderr, "Texture to big(limit %d), %dx%d.\n", g_maxtexsize, width, height);
		FreeImage_Unload(firsbitmap);
		FreeImage_Unload(bitmap);
		FreeImage_CloseMemory(stream);
		return -1;
	}

	/*	Check error.	*/
	if(pixel == NULL || size == 0 ){
		fprintf(stderr, "Failed getting pixel data from FreeImage.\n");
		FreeImage_Unload(firsbitmap);
		FreeImage_Unload(bitmap);
		FreeImage_CloseMemory(stream);
		return -1;
	}

	/*	Make a copy of pixel data.	*/
	desc->pixel = malloc(size);
	if(desc->pixel == NULL){
		fprintf(stderr, "Failed to allocate %d, %s.\n", size, strerror(errno));
	}
	memcpy(desc->pixel, pixel, size);


	/*	set image attributes.	*/
	desc->size = size;
	desc->width = width;
	desc->height = height;
	desc->bpp = bpp;


	/*	Release free image resources.	*/
	FreeImage_Unload(bitmap);
	FreeImage_Unload(firsbitmap);
	FreeImage_CloseMemory(stream);

	return totallen;
}



int swpLoadTextureFromMem(GLuint* tex, GLuint pbo, const swpTextureDesc* desc){

	GLuint intfor = desc->intfor;			/*	*/
	GLuint format = desc->format;			/*	*/
	GLuint imgdatatype = desc->imgdatatype;	/*	*/
	GLboolean status;						/*	*/
	GLenum err = 0;							/*	*/
	GLubyte* pbuf = NULL;					/*	*/

	/*	*/
	const void* pixel = desc->pixel;
	const unsigned int width = desc->width;
	const unsigned int height = desc->height;
	const unsigned int size = desc->size;

	PFNGLMAPBUFFERPROC glMapBufferARB = NULL;
	PFNGLBINDBUFFERARBPROC glBindBufferARB = NULL;
	PFNGLBUFFERDATAARBPROC glBufferDataARB = NULL;

	swpVerbosePrintf("Loading texture from pixel data.\n");

	/*	Get compression.	*/
	if(g_compression){
		switch(intfor){
		case GL_RGB:
			intfor = GL_COMPRESSED_RGB;
			break;
		case GL_RGBA:
			intfor = GL_COMPRESSED_RGBA;
			break;
		default:
			break;
		}
	}

	/*	TODO check if PBO is supported.	*/
	if(g_support_pbo){

		/**/
		glBindBufferARB = SDL_GL_GetProcAddress("glBindBufferARB");
		glMapBufferARB = SDL_GL_GetProcAddress("glMapBufferARB");
		glBufferDataARB = SDL_GL_GetProcAddress("glBufferDataARB");

		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
	#if defined(GLES2) || defined(GLES3)
		glBufferData(GL_PIXEL_UNPACK_BUFFER, size, pixel, GL_STREAM_COPY_ARB);
	#else

		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, size, NULL, GL_STREAM_DRAW_ARB);
		err = glGetError();
		if( err != GL_NO_ERROR){
			fprintf(stderr, "Error on glBufferData %d.\n", err);
			return 0;
		}
		pbuf = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,
				GL_WRITE_ONLY_ARB);

		if(pbuf == NULL){
			err = glGetError();
			fprintf(stderr, "Bad pointer %i\n", err);
			return 0;
		}
		swpVerbosePrintf("Copying %d bytes to PBO (%d MB).\n", size, size / (1024 * 1024));
		memcpy(pbuf, pixel, size);
		status = glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
		if( status != GL_TRUE){
			fprintf(stderr, "Error when unmapping pbo buffer, %d\n", glGetError());
		}
#endif
	}


	/*	Create texture.	*/
	if(glIsTexture(*tex) == GL_FALSE){
		glGenTextures(1, tex);
		glBindTexture(GL_TEXTURE_2D, *tex);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_PACK_ALIGNMENT, 4);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);

	}
	glBindTexture(GL_TEXTURE_2D, *tex);

	/*	Transfer pixel data.	*/
	if(g_support_pbo)
		glTexImage2D(GL_TEXTURE_2D, 0, intfor, width, height, 0, format, imgdatatype, (const void*)NULL);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, intfor, width, height, 0, format, imgdatatype, (const void*)pixel);

	glGenerateMipmap(GL_TEXTURE_2D);

	/*	*/
	glBindTexture(GL_TEXTURE_2D, 0);
	if(g_support_pbo)
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER, 0);

	/*	Release pixel data.	*/
	free(desc->pixel);

	return glIsTexture(*tex) == SDL_TRUE;
}


void swpSetWallpaper(SDL_Window* window){

	void* dekstopwin = NULL;

	SDL_SysWMinfo wm;
	SDL_VERSION(&wm.version); /* initialize info structure with SDL version info */


	/*	Get desktop window. Properly has to write a OS depended code.	*/
	dekstopwin;



	if( SDL_GetWindowWMInfo(window, &wm) ){
		SDL_DisplayMode mode;
		if(wm.subsystem == SDL_SYSWM_X11){
			SDL_GetDesktopDisplayMode(0, &mode);


		}

	}else{

	}
}

void swpSetFullscreen(SDL_Window* window, Uint32 fullscreen){

	//SDL_SetWindowDisplayMode(window, mode);
	SDL_SetWindowFullscreen(window, fullscreen);
}


void* swpCatchPipedTexture(void* phandle){

	/*	*/
	fd_set read_fd_set;
	int fd = *(int*)phandle;
	int numdesc = 10;
	int curdesc = 0;
	swpTextureDesc desc[10] = { {0} };
	SDL_Event event = {0};
	ssize_t status;
	int len;

	swpVerbosePrintf("Started %s thread.\n", SDL_GetThreadName(NULL) );

	/*	*/
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

	/*	Open FIFO file.	*/
	fd = open(g_fifopath, O_RDONLY);
	if(fd < 0){
		fprintf(stderr, "Failed to open fifo, %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/*	Initialize the set of active sockets. */
	FD_ZERO (&read_fd_set);
	FD_SET (fd, &read_fd_set);

	/*	*/
	for(;;){

		if ( (len = select (fd + 1, &read_fd_set, NULL, NULL, NULL) ) < 0){
			perror ("select");
			exit (EXIT_FAILURE);
		}else{

			if(FD_ISSET(fd, &read_fd_set) ){
				swpVerbosePrintf("Select event %d.\n", fd);

				if (flock(fd, LOCK_EX) != 0) {
					fprintf(stderr, "Failed to lock file, %s.\n", strerror(errno));
				}

				/*	Load data from stream.	*/
				status = swpReadPicFromfd(fd, &desc[curdesc]);

				/*	Unlock the */
				if (flock(fd, LOCK_UN) != 0) {
					fprintf(stderr, "Failed to unlock file, %s.\n", strerror(errno));
				}

				/*	Close file.	*/
				if( close(fd) != 0){
					fprintf(stderr, "Failed to close file, %s.\n", strerror(errno));
				}


				/*	Send event to the main thread that will process the data.	*/
				if(status > 0){
					event.type = SDL_USEREVENT;
					event.user.data1 = &desc[curdesc];
					SDL_PushEvent(&event);
				}
				curdesc = ( curdesc + 1 ) % numdesc;

				/*	Reopen the file.	*/
				fd = open(g_fifopath, O_RDONLY);

				FD_ZERO (&read_fd_set);
				FD_SET (fd, &read_fd_set);
			}
		}

	}/*	Infinity loop.*/

	return NULL;
}

void swpCatchSignal(int sig){

	SDL_Event event = {0};

	switch(sig){
	case SIGSEGV:
	case SIGILL:
		unlink(g_fifopath);
		exit(EXIT_FAILURE);
		break;
	case SIGPIPE:
		fprintf(stderr, "Sigpipe.\n");
		break;
	case SIGTERM:
	case SIGINT:
		/*	Send event to main thread SDL event handler to quit.	*/
		event.type = SDL_QUIT;
		SDL_PushEvent(&event);
		g_alive = SDL_FALSE;
		break;
	default:
		break;
	}
}

void swpRender(GLuint vao, SDL_Window* window, swpRenderingState* state){

	swpVerbosePrintf("Render view.\n");

	if(state->inTransition){

		GLuint prog;
		int glprindex = rand() % state->data.numshaders;	/*	TODO replace later*/

		prog = state->data.shaders[glprindex].prog;
		glUseProgram(prog);

		/*	*/
		if(state->data.shaders[glprindex].elapse < state->elapseTransition){
			state->inTransition = 0;
			glUseProgram(state->data.displayshader->prog);
		}
	}

	/*	Draw quad.	*/
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4 );
	glBindVertexArray(0);
	SDL_GL_SwapWindow(window);

}
