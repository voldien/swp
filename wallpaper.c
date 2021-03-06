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

/*	OpenGL ARB function pointers.	*/
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
PFNGLISVERTEXARRAYPROC glIsVertexArray = NULL;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = NULL;
PFNGLGENBUFFERSARBPROC glGenBuffersARB = NULL;
PFNGLISBUFFERARBPROC glIsBufferARB = NULL;
PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB = NULL;
PFNGLMAPBUFFERPROC glMapBufferARB = NULL;
PFNGLUNMAPBUFFERPROC glUnmapBufferARB = NULL;
PFNGLBINDBUFFERARBPROC glBindBufferARB = NULL;
PFNGLBUFFERDATAARBPROC glBufferDataARB = NULL;
PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallbackARB = NULL;
PFNGLDEBUGMESSAGECALLBACKAMDPROC glDebugMessageCallbackAMD = NULL;

PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB = NULL;
PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB = NULL;
PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB = NULL;

PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
PFNGLCREATESHADERPROC glCreateShader = NULL;
PFNGLSHADERSOURCEARBPROC glShaderSourceARB = NULL;
PFNGLCOMPILESHADERARBPROC glCompileShaderARB = NULL;
PFNGLATTACHSHADERPROC glAttachShader = NULL;
PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
PFNGLVALIDATEPROGRAMPROC glValidateProgram = NULL;
PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
PFNGLDETACHSHADERPROC glDetachShader = NULL;
PFNGLDELETESHADERPROC glDeleteShader = NULL;
PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation = NULL;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;


PFNGLGENERATEMIPMAPPROC glGenerateMipmap = NULL;
PFNGLUSEPROGRAMPROC glUseProgram = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB = NULL;
PFNGLUNIFORM1IARBPROC glUniform1iARB = NULL;
PFNGLUNIFORM1FARBPROC glUniform1fARB = NULL;
PFNGLUNIFORM1FVARBPROC glUniform1fvARB = NULL;
PFNGLPROGRAMUNIFORM1IPROC glProgramUniform1i = NULL;
PFNGLPROGRAMUNIFORM1FPROC glProgramUniform1f = NULL;

/*	Default vertex shader.	*/
const char* gc_vertex = ""
"#extension GL_ARB_explicit_attrib_location : enable\n"
"#if defined(GL_ARB_explicit_attrib_location)\n"
"layout(location = 0) in vec3 vertex;\n"
"#else\n"
"attribute vec3 vertex;\n"
"#endif\n"
"#if __VERSION__ > 120\n"
"smooth out vec2 uv;\n"
"#else\n"
"varying vec2 uv;\n"
"#endif\n"
"out gl_PerVertex{\n"
"    vec4 gl_Position;\n"
"    float gl_PointSize;\n"
"    float gl_ClipDistance[];\n"
"};\n"
"void main(void){\n"
"	gl_Position = vec4(vertex,1.0);\n"
"	uv = (vertex.xy + vec2(1.0)) / 2.0;"
"}\n";

/*	Default fragment shader.	*/
const char* gc_fragment = ""
"#extension GL_ARB_explicit_attrib_location : enable\n"
"#if defined(GL_ARB_explicit_attrib_location)\n"
"layout(location = 0) out vec4 fragColor;\n"
"#else\n"
"out vec4 fragColor;\n"
"#endif\n"
"uniform sampler2D tex0;\n"
"#if __VERSION__ > 120\n"
"smooth in vec2 uv;\n"
"#else\n"
"varying vec2 uv;\n"
"#endif\n"
"void main(void){\n"
"#if defined(GL_ARB_explicit_attrib_location)\n"
"   #if __VERSION__ > 120\n"
"	fragColor = texture(tex0, uv);\n"
"   #else\n"
"   fragColor = texture2D(tex0, uv);\n"
"   #endif\n"
"#else\n"
"   #if __VERSION__ > 120\n"
"	fragColor = texture(tex0, uv);\n"
"   #else\n"
"   gl_FragColor = texture2D(tex0, uv);\n"
"   #endif\n"
"#endif\n"
"}\n";

const char* gc_fade_transition_fragment = ""
"#extension GL_ARB_explicit_attrib_location : enable\n"
"#if defined(GL_ARB_explicit_attrib_location)\n"
"layout(location = 0) out vec4 fragColor;\n"
"#else\n"
"out vec4 fragColor;\n"
"#endif\n"
"uniform sampler2D tex0;\n"
"uniform sampler2D tex1;\n"
"uniform float normalizedur;\n"
"#if __VERSION__ > 120\n"
"smooth in vec2 uv;\n"
"#else\n"
"varying vec2 uv;\n"
"#endif\n"
"void main(void){\n"
"#if defined(GL_ARB_explicit_attrib_location)\n"
"   #if __VERSION__ > 120\n"
"	fragColor = mix(texture(tex1, uv), texture(tex0, uv), normalizedur);\n"
"	#else\n"
"	fragColor = mix(texture2D(tex1, uv), texture2D(tex0, uv), normalizedur);\n"
"	#endif\n"
"#else\n"
"   #if __VERSION__ > 120\n"
"	gl_FragColor = mix(texture(tex1, uv), texture(tex0, uv), normalizedur);\n"
"	#else\n"
"	gl_FragColor = mix(texture2D(tex1, uv), texture2D(tex0, uv), normalizedur);\n"
"	#endif\n"
"#endif\n"
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
unsigned int g_core_profile = 1;


int swpVerbosePrintf(const char *format, ...) {

	int status = -1;

	if (g_verbose != 0) {
		va_list vl;
		va_start(vl, format);
		status = vfprintf(stdout, format, vl);
		va_end(vl);
	}
	return status;
}

const char *swpGetVersion(void) {
	return SWP_STR_VERSION;
}

void callback_debug_gl(GLenum source, GLenum type, GLuint id, GLenum severity,
                       GLsizei length, const GLchar* message, GLvoid* userParam) {

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
	}

	/*	*/
	printf(severityString);

	/*	*/
	printf(message);
	printf("\n");
}

void swpLoadGLFunc(void) {

	/*	Load function pointer by their symbol name.	*/
	glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC) SDL_GL_GetProcAddress("glDebugMessageCallbackARB");
	glDebugMessageCallbackAMD = (PFNGLDEBUGMESSAGECALLBACKAMDPROC) SDL_GL_GetProcAddress("glDebugMessageCallbackAMD");

	/*	Get function address.	*/
	glGenVertexArrays = SDL_GL_GetProcAddress("glGenVertexArrays");
	glBindVertexArray = SDL_GL_GetProcAddress("glBindVertexArray");
	glIsVertexArray = SDL_GL_GetProcAddress("glIsVertexArray");
	glDeleteVertexArrays = SDL_GL_GetProcAddress("glDeleteVertexArrays");

	glGenBuffersARB = SDL_GL_GetProcAddress("glGenBuffersARB");
	glIsBufferARB = SDL_GL_GetProcAddress("glIsBufferARB");
	glDeleteBuffersARB = SDL_GL_GetProcAddress("glDeleteBuffersARB");
	glBindBufferARB = (PFNGLBINDBUFFERARBPROC) SDL_GL_GetProcAddress("glBindBufferARB");
	glMapBufferARB = SDL_GL_GetProcAddress("glMapBufferARB");
	glUnmapBufferARB = SDL_GL_GetProcAddress("glUnmapBufferARB");
	glBufferDataARB = SDL_GL_GetProcAddress("glBufferDataARB");

	/*	*/
	glEnableVertexAttribArrayARB = SDL_GL_GetProcAddress("glEnableVertexAttribArrayARB");
	glVertexAttribPointerARB = SDL_GL_GetProcAddress("glVertexAttribPointerARB");

	glBindAttribLocationARB = SDL_GL_GetProcAddress("glBindAttribLocationARB");

	/*	*/
	glCreateProgram = SDL_GL_GetProcAddress("glCreateProgram");
	glCreateShader = SDL_GL_GetProcAddress("glCreateShader");
	glShaderSourceARB = SDL_GL_GetProcAddress("glShaderSourceARB");
	glCompileShaderARB = SDL_GL_GetProcAddress("glCompileShaderARB");
	glAttachShader = SDL_GL_GetProcAddress("glAttachShader");
	glLinkProgram = SDL_GL_GetProcAddress("glLinkProgram");
	glValidateProgram = SDL_GL_GetProcAddress("glValidateProgram");
	glGetProgramiv = SDL_GL_GetProcAddress("glGetProgramiv");
	glDetachShader = SDL_GL_GetProcAddress("glDetachShader");
	glDeleteShader = SDL_GL_GetProcAddress("glDeleteShader");
	glBindFragDataLocation = SDL_GL_GetProcAddress("glBindFragDataLocation");
	glGetProgramInfoLog = SDL_GL_GetProcAddress("glGetProgramInfoLog");

	/*	*/
	glGenerateMipmap = SDL_GL_GetProcAddress("glGenerateMipmap");
	glUseProgram = SDL_GL_GetProcAddress("glUseProgram");
	glGetUniformLocationARB = SDL_GL_GetProcAddress("glGetUniformLocationARB");
	glUniform1iARB = SDL_GL_GetProcAddress("glUniform1iARB");
	glUniform1fARB = SDL_GL_GetProcAddress("glUniform1fARB");
	glUniform1fvARB = SDL_GL_GetProcAddress("glUniform1fvARB");

	glProgramUniform1i = SDL_GL_GetProcAddress("glProgramUniform1i");
	glProgramUniform1f = SDL_GL_GetProcAddress("glProgramUniform1f");

}

void swpEnableDebug(void) {

	/*	Check if function pointer exists!	*/
	if (!glDebugMessageCallbackAMD && !glDebugMessageCallbackARB) {
		fprintf(stderr, "Failed loading OpenGL Message callback enable functions.\n");
	}

	/*	Set Debug message callback.	*/
	if (glDebugMessageCallbackARB) {
		glDebugMessageCallbackARB((GLDEBUGPROCARB) callback_debug_gl, NULL);
	}
	if (glDebugMessageCallbackAMD) {
		glDebugMessageCallbackAMD((GLDEBUGPROCAMD) callback_debug_gl, NULL);
	}

	/*	Enable debug.	*/
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
}


void swpParseResolutionArgument(const char *arg, int *res) {

	char *lws;
	char *rws;
	char optarg[1024];
	memcpy(optarg, arg, strlen(arg) + 1);

	/*	Parse.	*/
	lws = strstr(optarg, "x");
	*lws = '\0';
	rws = (char *) (lws + 1);
	lws = optarg;
	res[0] = (int) strtol((const char *) lws, NULL, 10);
	res[1] = (int) strtol((const char *) rws, NULL, 10);
}


long int swpLoadFile(const char *cfilename, void **data) {

	FILE *f;            /*	*/
	long int pos;        /*	*/
	long int nBytes;    /*	*/

	f = fopen(cfilename, "rb");
	if (f == NULL) {
		fprintf(stderr, "Failed to open %s, %s.\n", cfilename, strerror(errno));
		return -1;
	}

	/*	Determine size of file.	*/
	fseek(f, 0, SEEK_END);
	pos = ftell(f);
	fseek(f, 0, SEEK_SET);

	/*	*/
	data[0] = malloc(pos);
	assert(data[0]);

	/*	*/
	nBytes = fread(data[0], 1, pos, f);

	/*	*/
	fclose(f);

	return nBytes;
}

long int swpLoadString(const char *__restrict__ cfilename,
                       void **__restrict__ data) {

	long int l;

	l = swpLoadFile(cfilename, data);
	if (l > 0) {
		*data = realloc(*data, l + 1);
		((char **) *data)[l] = '\0';
	}

	return l;
}

void swpGenerateQuad(GLuint *vao, GLuint *vbo) {

	swpVerbosePrintf("Generating display quad.\n");
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);


	glGenBuffersARB(1, vbo);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, *vbo);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(gc_quad), gc_quad, GL_STATIC_DRAW_ARB);

	glEnableVertexAttribArrayARB(0);
	glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, 12, NULL);

	glBindVertexArray(0);
}

unsigned int swpGetGLSLVersion(void) {
	int major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);

	/*  Lookup table.   */
	int glVersion = major * 100 + minor * 10;
	switch(glVersion){
		case 200:return 110;
		case 210:return 120;
		case 300:return 130;
		case 310:return 140;
		case 320:return 150;
		case 330:return 330;
		case 400:return 400;
		case 410:return 410;
		case 420:return 420;
		case 430:return 430;
		case 440:return 440;
		case 450:return 450;
		case 460:return 460;
			// Add the rest of the mapping as each new version is released.
		default:
			return 110;
	}
}

unsigned int swpCheckExtensionSupported(const char *extension) {

	int i;
	GLint k;
	PFNGLGETSTRINGIPROC glGetStringi = NULL;

	/*	*/
	glGetStringi = (PFNGLGETSTRINGIPROC) SDL_GL_GetProcAddress("glGetStringi");

	/*	*/
	if (glGetStringi) {

		/*	*/
		glGetIntegerv(GL_NUM_EXTENSIONS, &k);

		/*	Iterate each extensions.	*/
		for (i = 0; i < k; i++) {
			const GLubyte *nextension = glGetStringi(GL_EXTENSIONS, i);
			if (nextension) {
				if (strstr((const char *) nextension, extension))
					return 1;
			}
		}
	} else
		return (strstr((const char *) glGetString(GL_EXTENSIONS), extension) != NULL);

	return 0;
}


GLint swpCreateShader(const char *vshader, const char *fshader) {

	GLuint vs, fs;
	GLuint prog;
	GLuint vstatus, lstatus;
	const char *vsources[2];
	const char *fsources[2];
	char glversion[32];

	const char *strcore;

	/*	*/
	swpVerbosePrintf("Loading shader program.\n");

	/*	Check if core.	*/
	strcore = "";
	/*	Create version string.	*/
	memset(glversion, 0, sizeof(glversion));
	sprintf(glversion, "#version %d %s\n", swpGetGLSLVersion(), strcore);    /*	TODO evalute.*/
	vsources[0] = glversion;
	fsources[0] = glversion;

	/*	*/
	prog = glCreateProgram();

	if (vshader != NULL) {
		vsources[1] = vshader;
		vs = glCreateShader(GL_VERTEX_SHADER_ARB);
		glShaderSourceARB(vs, 2, vsources, NULL);
		glCompileShaderARB(vs);
		glAttachShader(prog, vs);
	}
	if (fshader != NULL) {
		fsources[1] = fshader;
		fs = glCreateShader(GL_FRAGMENT_SHADER_ARB);
		glShaderSourceARB(fs, 2, fsources, NULL);
		glCompileShaderARB(fs);
		glAttachShader(prog, fs);
	}

	/*	Link shader.	*/
	glLinkProgram(prog);
	glValidateProgram(prog);

	/*	Check status of linking and validate.	*/
	glGetProgramiv(prog, GL_LINK_STATUS, &lstatus);
	glGetProgramiv(prog, GL_VALIDATE_STATUS, &vstatus);

	/*	Check if link successfully.	*/
	if (lstatus == GL_FALSE) {
		char info[4096];
		glGetProgramInfoLog(prog, sizeof(info), NULL, &info[0]);
		fprintf(stderr, "%s\n", info);
		return -1;
	}
	if (vstatus == GL_FALSE) {
		char info[4096];
		glGetProgramInfoLog(prog, sizeof(info), NULL, &info[0]);
		fprintf(stderr, "%s\n", info);
	}

	/*	Bind shader attribute for legacy shader version.	*/
	glBindAttribLocationARB(prog, 0, "vertex");
	glBindFragDataLocation(prog, 0, "fragColor");

	/*	Detach shaderer object and release their resources.	*/
	glDetachShader(prog, vs);
	glDetachShader(prog, fs);
	glDeleteShader(vs);
	glDeleteShader(fs);

	return prog;
}

void swpLoadTransitionShaders(swpRenderingState *__restrict__ state,
                              unsigned int count, const char **__restrict__ filepaths) {

	int x;

	swpVerbosePrintf("Loading %d number of transition shaders.\n", count);

	/*	Iterate through each shader.	*/
	for (x = 0; x < count; x++) {
		void *fragdata = NULL;
		const char *fsource = filepaths[x];

		/*	Get new shader object index.	*/
		const int index = state->data.numshaders;
		swpTransitionShader *trans;

		/*	Allocate shader object.	*/
		state->data.numshaders++;
		state->data.shaders = realloc(state->data.shaders, state->data.numshaders * sizeof(swpTransitionShader));
		assert(state->data.shaders);
		trans = &state->data.shaders[index];

		/*	Load shader.	*/
		swpVerbosePrintf("Loading %s.\n", fsource);
		if (swpLoadFile(fsource, &fragdata) > 0) {
			swpCreateTransitionShaders(trans, fragdata);
		}
	}
}

int swpCreateTransitionShaders(swpTransitionShader *__restrict__ trans,
                               const char *__restrict__ source) {

	/*	Create shader and check if successfully.	*/
	GLint prog = swpCreateShader(gc_vertex, source);

	/*	Check error.	*/
	if (prog < 0)
		return 0;
	trans->prog = prog;

	/*	Default elapse time for transition shader.	*/
	trans->elapse = 1.120f;

	/*	Cache uniform variable memory index.	*/
	trans->normalizedurloc = glGetUniformLocationARB(trans->prog, "normalizedur");
	trans->texloc0 = glGetUniformLocationARB(trans->prog, "tex0");
	trans->texloc1 = glGetUniformLocationARB(trans->prog, "tex1");

	/*	Assign default transition shader values.	*/
	glUseProgram(trans->prog);
	glUniform1iARB(trans->texloc0, 0);
	glUniform1iARB(trans->texloc1, 1);
	glUniform1fARB(trans->normalizedurloc, 0.0f);

	/*	Success.	*/
	return 1;
}

swpTransitionShader *swpCreateDefaultTransitionShader(swpRenderingState *__restrict__ state) {

	swpTransitionShader *trans;

	/*	Create default transition shader.	*/
	state->data.numshaders++;
	state->data.shaders = realloc(state->data.shaders, state->data.numshaders * sizeof(swpTransitionShader));

	/*	*/
	assert(state->data.shaders);

	/*	*/
	trans = &state->data.shaders[state->data.numshaders - 1];

	/*	Load shader.	*/
	swpCreateTransitionShaders(trans, gc_fade_transition_fragment);

	return trans;
}

GLuint swpGetGLTextureFormat(unsigned int ffpic) {

	switch (ffpic) {
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


ssize_t swpReadPicFromfd(int fd, swpTextureDesc *desc) {

	/*	*/
	char inbuf[4096];                   /**/
	register ssize_t len = 0;           /**/
	register ssize_t totallen = 0;      /**/

	/*	Free image.	*/
	FREE_IMAGE_FORMAT imgtype;          /**/
	FREE_IMAGE_COLOR_TYPE colortype;    /**/
	FREE_IMAGE_TYPE imgt;               /**/
	FIMEMORY *stream;                   /**/
	FIBITMAP *firsbitmap;               /**/
	FIBITMAP *bitmap;                   /**/
	void *pixel;                        /**/

	/*	*/
	unsigned int width;
	unsigned int height;
	unsigned int bpp;
	unsigned int size;

	swpVerbosePrintf("Starting loading image.\n");

	/*	1 byte for the size in order, Because it crash otherwise if set to 0.	*/
	stream = FreeImage_OpenMemory(NULL, 1);
	if (stream == NULL) {
		fprintf(stderr, "Failed to open freeimage memory stream. \n");
	}


	/*	Read from file stream.	*/
	while ((len = read(fd, inbuf, sizeof(inbuf))) > 0) {
		if (len < 0) {
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
	if (firsbitmap == NULL) {
		fprintf(stderr, "Failed to create free-image from memory.\n");
		FreeImage_CloseMemory(stream);
		return -1;
	}

	/*	Reset to beginning of stream.	*/
	FreeImage_SeekMemory(stream, 0, SEEK_SET);
	imgt = FreeImage_GetImageType(firsbitmap);
	colortype = FreeImage_GetColorType(firsbitmap);

	/*	Input data type.	*/
	swpVerbosePrintf("Image pixel data type %d\n", imgt);
	desc->imgdatatype = swpGetGLTextureFormat(imgt);

	/*	Get texture color type.	*/
	swpVerbosePrintf("Image color type %d\n", colortype);
	switch (colortype) {
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
	if (bitmap == NULL) {
		fprintf(stderr, "Failed to convert bitmap.\n");
		FreeImage_CloseMemory(stream);
		FreeImage_Unload(firsbitmap);
		return -1;
	}

	/*	Get attributes from the image.	*/
	pixel = FreeImage_GetBits(bitmap);
	width = FreeImage_GetWidth(bitmap);
	height = FreeImage_GetHeight(bitmap);
	bpp = (FreeImage_GetBPP(bitmap) / 8);
	size = width * height * bpp;
	swpVerbosePrintf("%d kb, %d %dx%d\n", (size / 1024), imgtype, width, height);

	/*	Check size is supported by opengl driver.	*/
	if (width > g_maxtexsize || height > g_maxtexsize) {
		fprintf(stderr, "Texture to big(limit %d), %dx%d.\n", g_maxtexsize, width, height);
		FreeImage_Unload(firsbitmap);
		FreeImage_Unload(bitmap);
		FreeImage_CloseMemory(stream);
		return -1;
	}

	/*	Check error and release resources.	*/
	if (pixel == NULL || size == 0) {
		fprintf(stderr, "Failed getting pixel data from FreeImage.\n");
		FreeImage_Unload(firsbitmap);
		FreeImage_Unload(bitmap);
		FreeImage_CloseMemory(stream);
		return -1;
	}

	/*	Make a copy of pixel data.	*/
	desc->pixel = malloc(size);
	if (desc->pixel == NULL) {
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

int swpLoadTextureFromMem(GLuint *tex, GLuint pbo, const swpTextureDesc *desc) {

	GLuint intfor = desc->intfor;           /*	*/
	GLuint format = desc->format;           /*	*/
	GLuint imgdatatype = desc->imgdatatype; /*	*/
	GLboolean status;                       /*	*/
	GLenum err = 0;                         /*	*/
	GLubyte *pbuf = NULL;                   /*	*/

	/*	*/
	const void *pixel = desc->pixel;
	const unsigned int width = desc->width;
	const unsigned int height = desc->height;
	const unsigned int size = desc->size;

	swpVerbosePrintf("Loading texture from pixel data.\n");

	/*	Get compression.	*/
	if (g_compression) {
		switch (intfor) {
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


	if (g_support_pbo) {

		/*  Pop any existing error. */
		glGetError();

		/*  */
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
#if defined(GLES2) || defined(GLES3)
		glBufferData(GL_PIXEL_UNPACK_BUFFER, size, pixel, GL_STREAM_DRAW_ARB);
#else

		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, size, NULL, GL_STREAM_DRAW_ARB);
		err = glGetError();
		if (err != GL_NO_ERROR) {
			fprintf(stderr, "Error on glBufferData %d.\n", err);
			return 0;
		}
		pbuf = (GLubyte *) glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,
		                                  GL_WRITE_ONLY_ARB);

		if (pbuf == NULL) {
			err = glGetError();
			fprintf(stderr, "Bad pointer %i\n", err);
			return 0;
		}
		swpVerbosePrintf("Copying %d bytes to PBO (%d MB).\n", size, size / (1024 * 1024));
		memcpy(pbuf, pixel, size);
		status = glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
		if (status != GL_TRUE) {
			fprintf(stderr, "Error when unmapping pbo buffer, %d\n", glGetError());
		}
#endif
	}

	/*	Create texture.	*/
	if (glIsTexture(*tex) == GL_FALSE) {
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
	if (g_support_pbo)
		glTexImage2D(GL_TEXTURE_2D, 0, intfor, width, height, 0, format, imgdatatype, (const void *) NULL);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, intfor, width, height, 0, format, imgdatatype, (const void *) pixel);

	/*	*/
	glGenerateMipmap(GL_TEXTURE_2D);

	/*	*/
	glBindTexture(GL_TEXTURE_2D, 0);
	if (g_support_pbo)
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	/*	Release pixel data.	*/
	free(desc->pixel);

	return glIsTexture(*tex) == SDL_TRUE;
}


void swpSetWallpaper(SDL_Window *window) {

	void *dekstopwin = NULL;

	SDL_SysWMinfo wm;
	SDL_VERSION(&wm.version); /* initialize info structure with SDL version info */


	/*	Get desktop window. Properly has to write a OS depended code.	*/
	//dekstopwin;

	if (SDL_GetWindowWMInfo(window, &wm)) {
		SDL_DisplayMode mode;
		if (wm.subsystem == SDL_SYSWM_X11) {
			SDL_GetDesktopDisplayMode(0, &mode);


		}

	} 
	// else {

	// }
}

void swpSetFullscreen(SDL_Window *window, Uint32 fullscreen) {

	/*SDL_SetWindowDisplayMode(window, mode);	*/
	SDL_SetWindowFullscreen(window, fullscreen);
}


void *swpCatchPipedTexture(void *phandle) {

	/*	*/
	fd_set read_fd_set;
	int fd = *(int *) phandle;
	int numdesc = 10;
	int curdesc = 0;
	swpTextureDesc desc[10] = {{0}};
	SDL_Event event = {0};
	ssize_t status;
	int len;

	swpVerbosePrintf("Started %s thread.\n", SDL_GetThreadName(NULL));

	/*	*/
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

	/*	Open FIFO file.	*/
	fd = open(g_fifopath, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open fifo, %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/*	Initialize the set of active sockets. */
	FD_ZERO(&read_fd_set);
	FD_SET(fd, &read_fd_set);

	/*	*/
	for (;;) {

		if ((len = select(fd + 1, &read_fd_set, NULL, NULL, NULL)) < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		} else {

			if (FD_ISSET(fd, &read_fd_set)) {
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
				if (close(fd) != 0) {
					fprintf(stderr, "Failed to close file, %s.\n", strerror(errno));
				}


				/*	Send event to the main thread that will process the data.	*/
				if (status > 0) {
					event.user.code = SWP_EVENT_UPDATE_IMAGE;
					event.type = SDL_USEREVENT;
					event.user.data1 = &desc[curdesc];
					SDL_PushEvent(&event);
				}
				curdesc = (curdesc + 1) % numdesc;

				/*	Reopen the file.	*/
				fd = open(g_fifopath, O_RDONLY);

				FD_ZERO(&read_fd_set);
				FD_SET(fd, &read_fd_set);
			}
		}

	}/*	Infinity loop.*/

	return NULL;
}

void swpCatchSignal(int sig) {

	SDL_Event event = {0};

	switch (sig) {
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

void swpRender(GLuint vao, SDL_Window *__restrict__ window,
               swpRenderingState *__restrict__ state) {

	/*	*/
	if (state->inTransition != 0) {

		/*	*/
		const swpTransitionShader *trashader;
		int glprindex = state->data.numshaders - 1;
		float normalelapse;

		swpVerbosePrintf("Render Transition view.\n");

		/*	*/
		assert(state->data.numshaders > 1);

		/*	*/
		trashader = &state->data.shaders[glprindex];

		/*	*/
		glUseProgram(trashader->prog);

		/*	Update normalize elapse time.	*/
		normalelapse = (state->elapseTransition / state->data.shaders[glprindex].elapse);
		glUniform1fvARB(trashader->normalizedurloc, 1, &normalelapse);

		/*	Check if transition has ended.	*/
		if (state->elapseTransition > state->data.shaders[glprindex].elapse) {
			/*	Disable transition.	*/
			state->inTransition = 0;
			glUseProgram(state->data.shaders[0].prog);
		}

		/*	Update.	*/
		SDL_Event event = {0};
		event.type = SDL_USEREVENT;
		event.user.code = SWP_EVENT_UPDATE_TRANSITION;
		SDL_PushEvent(&event);
	} else
		swpVerbosePrintf("Render Non-Transition View.\n");

	/*	Draw quad.	*/
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	SDL_GL_SwapWindow(window);

}
