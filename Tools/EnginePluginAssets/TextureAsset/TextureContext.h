#pragma once

#include <EnginePluginAssets/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/Declarations.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;
class ezGameState;

class EZ_ENGINEPLUGINASSETS_DLL ezTextureContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureContext, ezEngineProcessDocumentContext);

public:
  ezTextureContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  void GetTextureStats(ezGALResourceFormat::Enum& format, ezUInt32& uiWidth, ezUInt32& uiHeight);

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  void OnResourceEvent(const ezResourceEvent& e);

  ezMaterialResourceHandle m_hMaterial;
  ezTextureResourceHandle m_hTexture;

  ezGALResourceFormat::Enum m_TextureFormat;
  ezUInt32 m_uiTextureWidth;
  ezUInt32 m_uiTextureHeight;
};


