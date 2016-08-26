#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderLoop/RenderLoop.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFrameDataProviderBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezFrameDataProviderBase::ezFrameDataProviderBase()
  : m_pOwnerPipeline(nullptr)
  , m_pData(nullptr)
  , m_uiLastUpdateFrame(0)
{

}

void* ezFrameDataProviderBase::GetData(const ezRenderViewContext& renderViewContext)
{
  if (m_pData == nullptr || m_uiLastUpdateFrame != ezRenderLoop::GetFrameCounter())
  {
    m_pData = UpdateData(renderViewContext, m_pOwnerPipeline->GetRenderData());

    m_uiLastUpdateFrame = ezRenderLoop::GetFrameCounter();
  }

  return m_pData;
}