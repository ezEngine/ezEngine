#pragma once

#include <RendererCore/Pipeline/RenderData.h>

class ezRmlUiRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiRenderData, ezRenderData);

public:
  ezGALTextureHandle m_hTexture;
  ezVec2 m_vOffset = ezVec2::MakeZero();
};
