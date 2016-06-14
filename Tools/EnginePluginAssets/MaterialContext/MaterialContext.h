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

class EZ_ENGINEPLUGINASSETS_DLL ezMaterialContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialContext, ezEngineProcessDocumentContext);

public:
  ezMaterialContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  ezMaterialResourceHandle m_hMaterial;
};


