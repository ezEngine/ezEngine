#include <RendererCore/PCH.h>
#include <RendererCore/RenderContext/RenderContext.h>

ezRenderContext* ezRenderContext::s_DefaultInstance = nullptr;
ezHybridArray<ezRenderContext*, 4> ezRenderContext::s_Instances;

ezRenderContext* ezRenderContext::GetDefaultInstance()
{
  if (s_DefaultInstance == nullptr)
    s_DefaultInstance = CreateInstance();

  return s_DefaultInstance;
}

ezRenderContext* ezRenderContext::CreateInstance()
{
  return EZ_DEFAULT_NEW(ezRenderContext);
}

void ezRenderContext::DestroyInstance(ezRenderContext* pRenderer)
{
  EZ_DEFAULT_DELETE(pRenderer);
}

ezRenderContext::ezRenderContext()
{
  if (s_DefaultInstance == nullptr)
  {
    SetGALContext(ezGALDevice::GetDefaultDevice()->GetPrimaryContext()); // set up with the default device
    s_DefaultInstance = this;
  }

  s_Instances.PushBack(this);

  m_StateFlags = ezRenderContextFlags::AllStatesInvalid;
  m_pCurrentlyModifyingBuffer = nullptr;
  m_uiMeshBufferPrimitiveCount = 0;
}

ezRenderContext::~ezRenderContext()
{
  if (s_DefaultInstance == this)
    s_DefaultInstance = nullptr;

  s_Instances.RemoveSwap(this);
}

void ezRenderContext::SetGALContext(ezGALContext* pContext)
{
  m_pGALContext = pContext;
}

void ezRenderContext::BindMeshBuffer(const ezMeshBufferResourceHandle& hMeshBuffer)
{
  if (m_hMeshBuffer == hMeshBuffer)
    return;

  m_StateFlags.Add(ezRenderContextFlags::MeshBufferBindingChanged);
  m_hMeshBuffer = hMeshBuffer;
}

ezRenderContext::Statistics::Statistics()
{
  Reset();
}

void ezRenderContext::Statistics::Reset()
{
  m_uiFailedDrawcalls = 0;
}