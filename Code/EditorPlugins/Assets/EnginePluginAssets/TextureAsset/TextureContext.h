#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
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

  const ezTexture2DResourceHandle& GetTexture() const { return m_hTexture; }

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  void OnResourceEvent(const ezResourceEvent& e);

  ezGameObjectHandle m_hPreviewObject;
  ezComponentHandle m_hPreviewMesh2D;
  ezMeshResourceHandle m_hPreviewMeshResource;
  ezMaterialResourceHandle m_hMaterial;
  ezTexture2DResourceHandle m_hTexture;

  ezEvent<const ezResourceEvent&, ezMutex>::Unsubscriber m_TextureResourceEventSubscriber;
};
