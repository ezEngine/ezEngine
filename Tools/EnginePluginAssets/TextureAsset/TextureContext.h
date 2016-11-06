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
  ezTextureContext() {}

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  ezMaterialResourceHandle m_hMaterial;
};


