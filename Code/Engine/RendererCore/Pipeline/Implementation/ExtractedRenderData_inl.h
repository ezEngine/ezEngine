#pragma once

void ezExtractedRenderData::AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category)
{
  m_DataPerCategory.EnsureCount(category.m_uiValue + 1);

  auto& sortableRenderData = m_DataPerCategory[category.m_uiValue].m_SortableRenderData.ExpandAndGetRef();
  sortableRenderData.m_pRenderData = pRenderData;
  sortableRenderData.m_uiSortingKey = pRenderData->GetFinalSortingKey(category, m_Camera);
}

void ezExtractedRenderData::AddFrameData(const ezRenderData* pFrameData)
{
  m_FrameData.PushBack(pFrameData);
}

void ezExtractedRenderData::AddViewDependency(ezGALTextureHandle hTexture, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage)
{
  if (hTexture.IsInvalidated())
    return;

  auto& dep = m_ViewTextureDependencies.ExpandAndGetRef();
  dep.m_hTexture = hTexture;
  dep.m_RequiredState = requiredState;
  dep.m_Stage = stage;
}

void ezExtractedRenderData::AddViewDependency(ezGALBufferHandle hBuffer, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage)
{
  if (hBuffer.IsInvalidated())
    return;

  auto& dep = m_ViewBufferDependencies.ExpandAndGetRef();
  dep.m_hBuffer = hBuffer;
  dep.m_RequiredState = requiredState;
  dep.m_Stage = stage;
}

void ezExtractedRenderData::AddDependency(const ezTextureDependency& dependency)
{
  EZ_ASSERT_DEBUG(dependency.m_uiCategory != ezInvalidRenderDataCategory.m_uiValue, "Per-category texture dependencies require a valid render data category. Use AddViewDependency for view-level dependencies.");
  m_DataPerCategory.EnsureCount(dependency.m_uiCategory + 1);
  m_DataPerCategory[dependency.m_uiCategory].m_TextureDependencies.PushBack(dependency);
}

void ezExtractedRenderData::AddDependency(const ezBufferDependency& dependency)
{
  EZ_ASSERT_DEBUG(dependency.m_uiCategory != ezInvalidRenderDataCategory.m_uiValue, "Per-category buffer dependencies require a valid render data category. Use AddViewDependency for view-level dependencies.");
  m_DataPerCategory.EnsureCount(dependency.m_uiCategory + 1);
  m_DataPerCategory[dependency.m_uiCategory].m_BufferDependencies.PushBack(dependency);
}

void ezExtractedRenderData::AddSamplerBinding(const ezSamplerBinding& binding)
{
  m_SamplerBindings.PushBack(binding);
}

void ezExtractedRenderData::AddBufferBinding(const ezBufferBinding& binding)
{
  m_BufferBindings.PushBack(binding);
}

void ezExtractedRenderData::AddTextureBinding(const ezTextureBinding& binding)
{
  m_TextureBindings.PushBack(binding);
}
