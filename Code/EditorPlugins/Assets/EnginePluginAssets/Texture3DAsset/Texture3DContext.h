#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;

class EZ_ENGINEPLUGINASSETS_DLL ezTexture3DContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTexture3DContext, ezEngineProcessDocumentContext);

public:
  ezTexture3DContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  const ezTexture3DResourceHandle& GetTexture() const { return m_hTexture; }

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  void SetTexture(ezStringView sTextureFile);
  void OnResourceEvent(const ezResourceEvent& e);
  void ApplyPreviewMode();

  ezGameObjectHandle m_hSlicePreviewObject;
  ezComponentHandle m_hSlicePreviewMesh;
  ezMeshResourceHandle m_hSlicePreviewMeshResource;
  ezMaterialResourceHandle m_hSliceMaterial;

  ezGameObjectHandle m_hRaymarchPreviewObject;
  ezComponentHandle m_hRaymarchPreviewMesh;
  ezMeshResourceHandle m_hRaymarchPreviewMeshResource;
  ezMaterialResourceHandle m_hRaymarchMaterial;

  ezTexture3DResourceHandle m_hTexture;

  // mirrors ezTexture3DPreviewMode (editor-side enum, not linked into this process): 0 = Slices, 1 = RayMarch
  ezInt32 m_iPreviewMode = 1;

  ezEvent<const ezResourceEvent&, ezMutex>::Unsubscriber m_TextureResourceEventSubscriber;
};
