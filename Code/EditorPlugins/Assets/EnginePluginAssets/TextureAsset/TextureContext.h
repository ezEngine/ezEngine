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
  int GetLodLevel() const { return m_iLodLevel; }
  ezUInt32 GetNumArraySlices() const { return m_uiNumArraySlices; }

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  void SetTexture(ezStringView sTextureFile);

  /// Applies all preview parameters to the slice materials. Does nothing and leaves
  /// m_bSliceMaterialsDirty set if the materials are not loaded yet, so that it gets retried.
  void ApplySliceMaterialParameters();
  void OnResourceEvent(const ezResourceEvent& e);
  void RebuildPreviewObjects(ezUInt32 uiNumArraySlices);

  struct PreviewSlice
  {
    ezGameObjectHandle m_hObject;
    ezComponentHandle m_hMeshComponent;
    ezMaterialResourceHandle m_hMaterial;
  };

  ezMeshResourceHandle m_hPreviewMeshResource;
  ezDynamicArray<PreviewSlice> m_SlicePreviews;

  ezTexture2DResourceHandle m_hTexture;
  ezEvent<const ezResourceEvent&, ezMutex>::Unsubscriber m_TextureResourceEventSubscriber;

  ezUInt32 m_uiNumArraySlices = 0;
  ezUInt32 m_uiMaterialGeneration = 0;
  bool m_bPreviewObjectsDirty = true;
  bool m_bSliceMaterialsDirty = true;
  int m_iLodLevel = -1;
  int m_iChannelMode = 0;
  float m_fAlphaThreshold = 0.0f;
};
