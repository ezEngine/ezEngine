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
    ezUInt32 m_uiTriangleCount = 0;
    ezGALBufferHandle m_hVertexBuffer;
    ezGALBufferHandle m_hIndexBuffer;
  };

  struct Batch
  {
    ezMat4 m_Transform = ezMat4::MakeIdentity();
    ezVec2 m_Translation = ezVec2(0);
    CompiledGeometry m_CompiledGeometry;
    ezTexture2DResourceHandle m_hTexture;
    ezRectFloat m_ScissorRect = ezRectFloat(0, 0);
    bool m_bEnableScissorRect = false;
    bool m_bTransformScissorRect = false;
  };
} // namespace ezRmlUiInternal

class ezRmlUiRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiRenderData, ezRenderData);

public:
  ezRmlUiRenderData(ezAllocator* pAllocator)
    : m_Batches(pAllocator)
  {
  }

  ezDynamicArray<ezRmlUiInternal::Batch> m_Batches;
};
