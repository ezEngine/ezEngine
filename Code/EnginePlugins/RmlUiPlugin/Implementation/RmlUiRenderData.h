#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Pipeline/RenderData.h>

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;

namespace ezRmlUiInternal
{
  struct Vertex
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_Position;
    ezVec2 m_TexCoord;
    ezColorLinearUB m_Color;
  };

  struct CompiledGeometry
  {
    ezGALBufferHandle m_hVertexBuffer;
    ezGALBufferHandle m_hIndexBuffer;
    ezTexture2DResourceHandle m_hTexture;
  };

  struct Batch
  {
    ezMat4 m_Transform = ezMat4::IdentityMatrix();
    CompiledGeometry m_CompiledGeometry;
    ezRectFloat m_ScissorRect;
  };
} // namespace ezRmlUiInternal

class ezRmlUiRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiRenderData, ezRenderData);

public:
  ezArrayPtr<ezRmlUiInternal::Batch> m_Batches;
};
