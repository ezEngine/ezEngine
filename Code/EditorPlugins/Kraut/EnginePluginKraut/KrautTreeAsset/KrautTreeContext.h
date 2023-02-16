#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginKraut/EnginePluginKrautDLL.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <RendererCore/Meshes/MeshResource.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;

class EZ_ENGINEPLUGINKRAUT_DLL ezKrautTreeContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeContext, ezEngineProcessDocumentContext);

public:
  ezKrautTreeContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;
  const ezKrautGeneratorResourceHandle& GetResource() const { return m_hMainResource; }

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);

  ezGameObject* m_pMainObject;
  ezComponentHandle m_hKrautComponent;
  ezKrautGeneratorResourceHandle m_hMainResource;
  ezMeshResourceHandle m_hPreviewMeshResource;
  ezUInt32 m_uiDisplayRandomSeed = 0xFFFFFFFF;
};
