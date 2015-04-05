#include <RendererCore/PCH.h>
#include <RendererCore/RendererCore.h>

ezRendererCore* ezRendererCore::s_DefaultInstance = nullptr;
ezHybridArray<ezRendererCore*, 4> ezRendererCore::s_Instances;

ezRendererCore* ezRendererCore::GetDefaultInstance()
{
  if (s_DefaultInstance == nullptr)
    s_DefaultInstance = CreateInstance();

  return s_DefaultInstance;
}

ezRendererCore* ezRendererCore::CreateInstance()
{
  return EZ_DEFAULT_NEW(ezRendererCore);
}

void ezRendererCore::DestroyInstance(ezRendererCore* pRenderer)
{
  EZ_DEFAULT_DELETE(pRenderer);
}

ezRendererCore::ezRendererCore()
{
  if (s_DefaultInstance == nullptr)
    s_DefaultInstance = this;

  s_Instances.PushBack(this);

  SetGALContext(ezGALDevice::GetDefaultDevice()->GetPrimaryContext()); // set up with the default device
}

ezRendererCore::~ezRendererCore()
{
  if (s_DefaultInstance == this)
    s_DefaultInstance = nullptr;

  s_Instances.RemoveSwap(this);
}

void ezRendererCore::SetGALContext(ezGALContext* pContext)
{
  m_pGALContext = pContext;
}