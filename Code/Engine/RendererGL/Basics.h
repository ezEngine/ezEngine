#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Basics.h>

// Different configuration possibilities:
//#define EZ_RENDERERGL_GL4
//#define EZ_RENDERERGL_GL3
//#define EZ_RENDERERGL_GLES3

// DLL/lib setup.
#if defined(BUILDSYSTEM_BUILDING_RENDERERGL4_LIB) || defined(BUILDSYSTEM_BUILDING_RENDERERGL3_LIB) || defined(BUILDSYSTEM_BUILDING_RENDERERGLES3_LIB)
  #define BUILDSYSTEM_BUILDING_RENDERERGL_LIB
#endif

#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_RENDERERGL_LIB
    #define EZ_RENDERERGL_DLL __declspec(dllexport)
    #define GLEW_BUILD
  #else
    #define EZ_RENDERERGL_DLL __declspec(dllimport)
  #endif
#else
  #define EZ_RENDERERGL_DLL
  #define GLEW_STATIC
#endif


// Performs OpenGL error handling via glGetError and outputs results into ezLog.
extern ezResult ezGALShaderGLCheckError(const char* szOperationName);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

  // Internal macro that defines what happens on an OpenGL call.
  #define __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, ...) \
  { \
    if ((pOpenGLFunction) == nullptr) \
    { \
      ezLog::Error("OpenGL function \"%s\" is not available!", szOpenGLFunctionName); \
      return EZ_FAILURE; \
    } \
    pOpenGLFunction(__VA_ARGS__); \
    return ezGALShaderGLCheckError(szOpenGLFunctionName); \
  }

  #define __EZ_GL_CALL_HANDLING_RET(szOpenGLFunctionName, pOpenGLFunction, ...) \
  { \
    if ((pOpenGLFunction) == nullptr) \
    { \
      ezLog::Error("OpenGL function \"%s\" is not available!", szOpenGLFunctionName); \
      return EZ_FAILURE; \
    } \
    ezUInt32 out = pOpenGLFunction(__VA_ARGS__); \
    ezGALShaderGLCheckError(szOpenGLFunctionName); \
    return out; \
  }

#else

  // Internal macro that defines what happens on an OpenGL call.
  #define __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, ...) \
  { \
    pOpenGLFunction(__VA_ARGS__); \
    return EZ_SUCCESS; \
  }

  #define __EZ_GL_CALL_HANDLING_RET(szOpenGLFunctionName, pOpenGLFunction, ...) \
  { \
    return pOpenGLFunction(__VA_ARGS__); \
  }

#endif

// GL call helper. The advantage of template function instead of a mere macro is that it can also be used in conditions!
// The whole thing would be of course much easier with variadic templates. But MSVC <120 lacking support forces us to do this.
template<typename GlFunction, typename Arg0>
ezResult ezGLCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction, Arg0 arg0) __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, arg0);
template<typename GlFunction, typename Arg0, typename Arg1>
ezResult ezGLCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction, Arg0 arg0, Arg1 arg1) __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, arg0, arg1);
template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2>
ezResult ezGLCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2) __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, arg0, arg1, arg2);
template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3>
ezResult ezGLCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3) __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, arg0, arg1, arg2, arg3);
template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
ezResult ezGLCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, arg0, arg1, arg2, arg3, arg4);
template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
ezResult ezGLCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5) __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, arg0, arg1, arg2, arg3, arg4, arg5);
template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
ezResult ezGLCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6) __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
ezResult ezGLCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7) __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8>
ezResult ezGLCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8) __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9>
ezResult ezGLCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9) __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
template<typename GlFunction, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9, typename Arg10>
ezResult ezGLCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction, Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9, Arg10 arg10) __EZ_GL_CALL_HANDLING(szOpenGLFunctionName, pOpenGLFunction, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);


template<typename GlFunction>
ezUInt32 ezGLIsCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction) __EZ_GL_CALL_HANDLING_RET(szOpenGLFunctionName, pOpenGLFunction);
template<typename GlFunction, typename Arg0>
ezUInt32 ezGLIsCall(const char* szOpenGLFunctionName, GlFunction pOpenGLFunction, Arg0 arg0) __EZ_GL_CALL_HANDLING_RET(szOpenGLFunctionName, pOpenGLFunction, arg0);

// Remove internal call handling helper macro.
#undef __EZ_GL_CALL_HANDLING
#undef __EZ_GL_CALL_HANDLING_RET

// Recommend way to call any OpenGL function. Will perform optional nullptr and glGetError checks.
#define EZ_GL_CALL(OpenGLFunction, ...) \
  ezGLCall(EZ_STRINGIZE(OpenGLFunction), OpenGLFunction, __VA_ARGS__)

// There are a few functions that have a return value (glGet, glIsX, glCreateShader, glCreateProgram). Use this macro for those.
#define EZ_GL_RET_CALL(OpenGLFunction, ...) \
  ezGLIsCall(EZ_STRINGIZE(OpenGLFunction), OpenGLFunction, __VA_ARGS__)


// Typedefs for different kinds of OpenGL ids for type safety.
typedef ezUInt32 glShaderId;
typedef ezUInt32 glProgramId;

typedef ezUInt32 glBufferId;
typedef ezUInt32 glVertexArrayObjectId;

typedef ezUInt32 glTextureId;
typedef ezUInt32 glFramebuffer;
typedef ezUInt32 glSamplerId;

typedef ezUInt32 glBindingTarget;

typedef ezUInt32 glQueryId;

// Value for uninitialized or deleted gl handles.
#define EZ_RENDERERGL_INVALID_ID 0xfefefefe
