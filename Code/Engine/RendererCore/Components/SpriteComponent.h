#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

class EZ_RENDERERCORE_DLL ezSpriteRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpriteRenderData, ezRenderData);

public:
  ezTexture2DResourceHandle m_hTexture;

  float m_fSize;
  float m_fMaxScreenSize;
  ezColorLinearUB m_color;

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
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds) override;

  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void SetTexture(const ezTexture2DResourceHandle& hTexture);
  const ezTexture2DResourceHandle& GetTexture() const;

  void SetTextureFile(const char* szFile);
  const char* GetTextureFile() const;

  void SetColor(ezColorGammaUB color);
  ezColorGammaUB GetColor() const;

  void SetSize(float fSize);
  float GetSize() const;

  void SetMaxScreenSize(float fSize);
  float GetMaxScreenSize() const;

private:

  ezTexture2DResourceHandle m_hTexture;
  ezColorGammaUB m_Color;

  float m_fSize;
  float m_fMaxScreenSize;
};

