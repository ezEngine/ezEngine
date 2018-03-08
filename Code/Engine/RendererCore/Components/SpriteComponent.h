#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>

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

public:
  ezSpriteComponent();
  ~ezSpriteComponent();

  // ezRenderComponent interface
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void SetTexture(const ezTexture2DResourceHandle& hTexture);
  const ezTexture2DResourceHandle& GetTexture() const;

  void SetTextureFile(const char* szFile);
  const char* GetTextureFile() const;

  void SetColor(ezColor color);
  ezColor GetColor() const;

  void SetSize(float fSize);
  float GetSize() const;

  void SetMaxScreenSize(float fSize);
  float GetMaxScreenSize() const;

  void OnSetColor(ezMsgSetColor& msg);

private:

  ezTexture2DResourceHandle m_hTexture;
  ezEnum<ezSpriteBlendMode> m_BlendMode;
  ezColor m_Color;

  float m_fSize;
  float m_fMaxScreenSize;
  float m_fAspectRatio;
};

