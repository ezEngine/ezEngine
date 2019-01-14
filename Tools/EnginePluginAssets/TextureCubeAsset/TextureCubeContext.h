#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/Plugin.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;

class EZ_ENGINEPLUGINASSETS_DLL ezTextureCubeContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureCubeContext, ezEngineProcessDocumentContext);

public:
  ezTextureCubeContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  void GetTextureStats(ezGALResourceFormat::Enum& format, ezUInt32& uiWidthAndHeight);

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
  ezTextureCubeResourceHandle m_hTexture;

  ezGALResourceFormat::Enum m_TextureFormat;
  ezUInt32 m_uiTextureWidthAndHeight;
  bool m_bAddedEventHandler;
};
