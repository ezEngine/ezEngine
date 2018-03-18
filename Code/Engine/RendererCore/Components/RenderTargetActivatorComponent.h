#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

struct ezMsgExtractRenderData;

typedef ezComponentManager<class ezRenderTargetActivatorComponent, ezBlockStorageType::Compact> ezRenderTargetComponentManager;

class EZ_RENDERERCORE_DLL ezRenderTargetActivatorComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRenderTargetActivatorComponent, ezRenderComponent, ezRenderTargetComponentManager);

public:
  ezRenderTargetActivatorComponent();
  ~ezRenderTargetActivatorComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
public:

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent Interface

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderTargetActivatorComponent Interface

public:

  void SetTexture(const ezTexture2DResourceHandle& hResource);
  ezTexture2DResourceHandle GetTexture() const { return m_hTexture; }

  void SetTextureFile(const char* szFile);
  const char* GetTextureFile() const;

private:
  ezTexture2DResourceHandle m_hTexture;
};

