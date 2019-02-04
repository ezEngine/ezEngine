#include <PCH.h>

#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFrameDataProviderBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezFrameDataProviderBase::ezFrameDataProviderBase()
    : m_pOwnerPipeline(nullptr)
    , m_pData(nullptr)
    , m_uiLastUpdateFrame(0)
{
}

void* ezFrameDataProviderBase::GetData(const ezRenderViewContext& renderViewContext)
{
  if (m_pData == nullptr || m_uiLastUpdateFrame != ezRenderWorld::GetFrameCounter())
  {
    m_pData = UpdateData(renderViewContext, m_pOwnerPipeline->GetRenderData());

    m_uiLastUpdateFrame = ezRenderWorld::GetFrameCounter();
  }

  return m_pData;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_FrameDataProvider);

