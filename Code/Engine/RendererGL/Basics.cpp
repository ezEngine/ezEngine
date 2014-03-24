#include "PCH.h"

#include <Foundation/Logging/Log.h>

#include <RendererGL/glew/glew.h>

ezResult ezGALShaderGLCheckError(const char* szOperationName)
{
  GLenum Error = glGetError();
  if (Error != GL_NO_ERROR)
  {
    const char* szErrorString;
    const char* szDescription;
    switch (Error)
    {
    case GL_INVALID_ENUM:
      szErrorString = "GL_INVALID_ENUM";
      szDescription = "An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.";
      break;

    case GL_INVALID_VALUE:
      szErrorString = "GL_INVALID_VALUE";
      szDescription = "A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.";
      break;

    case GL_INVALID_OPERATION:
      szErrorString = "GL_INVALID_OPERATION";
      szDescription = "The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.";
      break;

    case GL_INVALID_FRAMEBUFFER_OPERATION:
      szErrorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
      szDescription = "The command is trying to render to or read from the framebuffer while the currently bound framebuffer is not framebuffer complete. "
                      "The offending command is ignored and has no other side effect than to set the error flag.";
      break;

    case GL_OUT_OF_MEMORY:
      szErrorString = "GL_OUT_OF_MEMORY";
      szDescription = "There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
      break;

    default:
      szErrorString = "UNKNOWN";
      szDescription = "Unknown error code.";
      break;
    }
    ezLog::Error("OpenGL Error during %s: %s (%s)", szOperationName, szErrorString, szDescription);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}