#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

struct ezTextSpriteBlendMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Masked,
    Transparent,
    Additive,

    Default = Masked
  };

  static ezTempHashedString GetPermutationValue(Enum blendMode);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezTextSpriteBlendMode);

class EZ_RENDERERCORE_DLL ezTextSpriteRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextSpriteRenderData, ezRenderData);

public:
  void FillBatchIdAndSortingKey();

  ezTexture2DResourceHandle m_hTexture;

  float m_fSize;
  float m_fMaxScreenSize;
  float m_fAspectRatio;
  ezEnum<ezTextSpriteBlendMode> m_BlendMode;

  ezColor m_color;

  ezVec2 m_texCoordScale;
  ezVec2 m_texCoordOffset;

  ezUInt32 m_uiUniqueID;
};
