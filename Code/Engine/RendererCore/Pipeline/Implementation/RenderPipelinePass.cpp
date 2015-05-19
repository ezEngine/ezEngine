#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>

ezRenderPipelinePass::ezRenderPipelinePass(const char* szName)
{
  m_sName.Assign(szName);
  m_ProfilingID = ezProfilingSystem::CreateId(szName);

  m_pPipeline = nullptr;
}

ezRenderPipelinePass::~ezRenderPipelinePass()
{
}

void ezRenderPipelinePass::AddRenderer(ezUniquePtr<ezRenderer>&& pRenderer)
{
  ezHybridArray<const ezRTTI*, 8> supportedTypes;
  pRenderer->GetSupportedRenderDataTypes(supportedTypes);

  ezUInt32 uiIndex = m_Renderer.GetCount();
  m_Renderer.PushBack(std::move(pRenderer));

  for (ezUInt32 i = 0; i < supportedTypes.GetCount(); ++i)
  {
    m_TypeToRendererIndex.Insert(supportedTypes[i], uiIndex);
  }
}

void ezRenderPipelinePass::Run(const ezRenderViewContext& renderViewContext)
{
  EZ_PROFILE(m_ProfilingID);

  Execute(renderViewContext);
}

void ezRenderPipelinePass::RenderDataWithPassType(const ezRenderViewContext& renderViewContext, ezRenderPassType passType)
{
  EZ_PROFILE(m_pPipeline->GetPassTypeProfilingID(passType));

  ezArrayPtr<const ezRenderData*> renderData = m_pPipeline->GetRenderDataWithPassType(passType);
  while (renderData.GetCount() > 0)
  {
    const ezRenderData* pRenderData = renderData[0];
    const ezRTTI* pType = pRenderData->GetDynamicRTTI();
    const ezUInt32 uiDataLeft = renderData.GetCount();
    ezUInt32 uiDataRendered = 1;

    ezUInt32 uiRendererIndex = ezInvalidIndex;
    if (m_TypeToRendererIndex.TryGetValue(pType, uiRendererIndex))
    {
      uiDataRendered = m_Renderer[uiRendererIndex]->Render(renderViewContext, this, renderData);
    }
    else
    {
      ezLog::Warning("Could not render object of type '%s' in render pass '%s'. No suitable renderer found.", pType->GetTypeName(), m_sName.GetString().GetData());
     
      while (uiDataRendered < uiDataLeft && renderData[uiDataRendered]->GetDynamicRTTI() == pType)
      {
        ++uiDataRendered;
      }      
    }

    renderData = renderData.GetSubArray(uiDataRendered, uiDataLeft - uiDataRendered);
  }  
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelinePass);

