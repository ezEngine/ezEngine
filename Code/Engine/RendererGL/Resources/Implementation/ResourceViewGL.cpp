#include <RendererGL/PCH.h>
#include <RendererGL/Resources/ResourceViewGL.h>

ezGALResourceViewGL::ezGALResourceViewGL(const ezGALResourceViewCreationDescription& Description)
  : ezGALResourceView(Description)
{
}

ezGALResourceViewGL::~ezGALResourceViewGL()
{
}

ezResult ezGALResourceViewGL::InitPlatform(ezGALDevice* pDevice) 
{
  return EZ_SUCCESS;
}

ezResult ezGALResourceViewGL::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}