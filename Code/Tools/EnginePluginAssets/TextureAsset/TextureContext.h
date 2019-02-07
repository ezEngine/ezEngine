#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;

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

  void UpdatePreview();

  ezGameObjectHandle m_hPreviewObject;
  ezComponentHandle m_hPreviewMesh2D;
  ezMaterialResourceHandle m_hMaterial;
  ezTexture2DResourceHandle m_hTexture;

  ezGALResourceFormat::Enum m_TextureFormat;
  ezUInt32 m_uiTextureWidth;
  ezUInt32 m_uiTextureHeight;
  bool m_bAddedEventHandler;
};
