#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

typedef ezTypedResourceHandle<class ezTextureResource> ezTextureResourceHandle;

class EZ_RENDERERCORE_DLL ezSpriteRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpriteRenderData, ezRenderData);

public:
  ezTextureResourceHandle m_hTexture;

  float m_fSize;
  float m_fMaxScreenSize;
  ezColorLinearUB m_color;

  ezVec2 m_texCoordScale;
  ezVec2 m_texCoordOffset;

  ezUInt32 m_uiEditorPickingID;
};

class ezSpriteComponent;
typedef ezComponentManager<ezSpriteComponent, true> ezSpriteComponentManager;

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

  void SetTexture(const ezTextureResourceHandle& hTexture);
  const ezTextureResourceHandle& GetTexture() const;

  void SetTextureFile(const char* szFile);
  const char* GetTextureFile() const;

  void SetColor(ezColor color);
  ezColor GetColor() const;

  void SetSize(float fSize);
  float GetSize() const;

  void SetMaxScreenSize(float fSize);
  float GetMaxScreenSize() const;

private:

  ezTextureResourceHandle m_hTexture;
  ezColor m_Color;

  float m_fSize;
  float m_fMaxScreenSize;
};

