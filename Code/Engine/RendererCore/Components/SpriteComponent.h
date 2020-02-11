#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezMsgSetColor;
typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

struct ezSpriteBlendMode
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

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSpriteBlendMode);

class EZ_RENDERERCORE_DLL ezSpriteRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpriteRenderData, ezRenderData);

public:
  void FillBatchIdAndSortingKey();

  ezTexture2DResourceHandle m_hTexture;

  float m_fSize;
  float m_fMaxScreenSize;
  float m_fAspectRatio;
  ezEnum<ezSpriteBlendMode> m_BlendMode;

  ezColor m_color;

  ezVec2 m_texCoordScale;
  ezVec2 m_texCoordOffset;

  ezUInt32 m_uiUniqueID;
};

typedef ezComponentManager<class ezSpriteComponent, ezBlockStorageType::Compact> ezSpriteComponentManager;

class EZ_RENDERERCORE_DLL ezSpriteComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSpriteComponent, ezRenderComponent, ezSpriteComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;


  //////////////////////////////////////////////////////////////////////////
  // ezSpriteComponent

public:
  ezSpriteComponent();
  ~ezSpriteComponent();

  void SetTexture(const ezTexture2DResourceHandle& hTexture);
  const ezTexture2DResourceHandle& GetTexture() const;

  void SetTextureFile(const char* szFile); // [ property ]
  const char* GetTextureFile() const;      // [ property ]

  void SetColor(ezColor color); // [ property ]
  ezColor GetColor() const;     // [ property ]

  void SetSize(float fSize); // [ property ]
  float GetSize() const;     // [ property ]

  void SetMaxScreenSize(float fSize); // [ property ]
  float GetMaxScreenSize() const;     // [ property ]

  void OnMsgSetColor(ezMsgSetColor& msg); // [ property ]

private:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezTexture2DResourceHandle m_hTexture;
  ezEnum<ezSpriteBlendMode> m_BlendMode;
  ezColor m_Color = ezColor::White;

  float m_fSize = 1.0f;
  float m_fMaxScreenSize = 64.0f;
  float m_fAspectRatio = 1.0f;
};
