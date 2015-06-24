
template <typename T>
T* ezRenderPipeline::CreateRenderData(ezRenderPassType passType, ezGameObject* pOwner)
{
  EZ_CHECK_AT_COMPILETIME(EZ_IS_DERIVED_FROM_STATIC(ezRenderData, T));

  T* pRenderData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), T);
  pRenderData->m_uiSortingKey = 0; /// \todo implement sorting

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  pRenderData->m_pOwner = pOwner;
#endif
    
  PipelineData* pPipelineData = GetPipelineDataForExtraction();
  if (passType >= pPipelineData->m_PassData.GetCount())
  {
    pPipelineData->m_PassData.SetCount(passType + 1);
  }

  pPipelineData->m_PassData[passType].m_RenderData.PushBack(pRenderData);

  return pRenderData;
}

EZ_FORCE_INLINE ezArrayPtr<const ezRenderData*> ezRenderPipeline::GetRenderDataWithPassType(ezRenderPassType passType)
{
  ezArrayPtr<const ezRenderData*> renderData;

  PipelineData* pPipelineData = GetPipelineDataForRendering();
  if (pPipelineData->m_PassData.GetCount() > passType)
  {
    renderData = pPipelineData->m_PassData[passType].m_RenderData;
  }

  return renderData;
}

// static
EZ_FORCE_INLINE const char* ezRenderPipeline::GetPassTypeName(ezRenderPassType passType)
{
  return s_PassTypeData[passType].m_sName.GetString().GetData();
}

// static
EZ_FORCE_INLINE ezProfilingId& ezRenderPipeline::GetPassTypeProfilingID(ezRenderPassType passType)
{
  return s_PassTypeData[passType].m_ProfilingID;
}
