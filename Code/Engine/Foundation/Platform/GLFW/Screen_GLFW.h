#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/Screen.h>
#include <GLFW/glfw3.h>

namespace
{
  ezResult ezGlfwError(const char* file, size_t line)
  {
    const char* desc;
    int errorCode = glfwGetError(&desc);
    if (errorCode != GLFW_NO_ERROR)
    {
      ezLog::Error("GLFW error {} ({}): {} - {}", file, line, errorCode, desc);
      return EZ_FAILURE;
    }
    return EZ_SUCCESS;
  }
} // namespace

#define EZ_GLFW_RETURN_FAILURE_ON_ERROR()         \
  do                                              \
  {                                               \
    if (ezGlfwError(__FILE__, __LINE__).Failed()) \
      return EZ_FAILURE;                          \
  } while (false)

ezResult ezScreen::EnumerateScreens(ezDynamicArray<ezScreenInfo>& out_Screens)
{
  out_Screens.Clear();

  int iMonitorCount = 0;
  GLFWmonitor** pMonitors = glfwGetMonitors(&iMonitorCount);
  EZ_GLFW_RETURN_FAILURE_ON_ERROR();
  if (iMonitorCount == 0)
  {
    return EZ_FAILURE;
  }

  GLFWmonitor* pPrimaryMonitor = glfwGetPrimaryMonitor();
  EZ_GLFW_RETURN_FAILURE_ON_ERROR();
  if (pPrimaryMonitor == nullptr)
  {
    return EZ_FAILURE;
  }

  for (int i = 0; i < iMonitorCount; ++i)
  {
    ezScreenInfo& screen = out_Screens.ExpandAndGetRef();
    screen.m_sDisplayName = glfwGetMonitorName(pMonitors[i]);
    EZ_GLFW_RETURN_FAILURE_ON_ERROR();

    const GLFWvidmode* mode = glfwGetVideoMode(pMonitors[i]);
    EZ_GLFW_RETURN_FAILURE_ON_ERROR();
    if (mode == nullptr)
    {
      return EZ_FAILURE;
    }
    screen.m_iResolutionX = mode->width;
    screen.m_iResolutionY = mode->height;

    glfwGetMonitorPos(pMonitors[i], &screen.m_iOffsetX, &screen.m_iOffsetY);
    EZ_GLFW_RETURN_FAILURE_ON_ERROR();

    screen.m_bIsPrimary = pMonitors[i] == pPrimaryMonitor;
  }

  return EZ_SUCCESS;
}
