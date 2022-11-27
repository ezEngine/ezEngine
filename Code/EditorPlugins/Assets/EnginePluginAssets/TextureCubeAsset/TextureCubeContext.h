#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
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

  const ezTextureCubeResourceHandle& GetTexture() const { return m_hTexture; }

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
  ezTextureCubeResourceHandle m_hTexture;

  ezEvent<const ezResourceEvent&, ezMutex>::Unsubscriber m_TextureResourceEventSubscriber;
};
