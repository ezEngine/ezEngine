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

class EZ_ENGINEPLUGINASSETS_DLL ezMaterialContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialContext, ezEngineProcessDocumentContext);

public:
  ezMaterialContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

private:
  ezMaterialResourceHandle m_hMaterial;
  ezMeshResourceHandle m_hBallMesh;
  ezMeshResourceHandle m_hSphereMesh;
  ezMeshResourceHandle m_hBoxMesh;
  ezMeshResourceHandle m_hPlaneMesh;
  ezGameObjectHandle m_hMeshObject;
  ezComponentHandle m_hMeshComponent;

  enum class PreviewModel : ezUInt8
  {
    Ball,
    Sphere,
    Box,
    Plane,
  };

  PreviewModel m_PreviewModel = PreviewModel::Ball;
};
