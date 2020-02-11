#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <RendererCore/Meshes/MeshResource.h>

class EZ_ENGINEPLUGINASSETS_DLL ezDecalContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalContext, ezEngineProcessDocumentContext);

public:
  ezDecalContext();

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  ezMeshResourceHandle m_hPreviewMeshResource;

  //ezDecalResourceHandle m_hDecal;
};


