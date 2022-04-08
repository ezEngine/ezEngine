#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginJolt/EnginePluginJoltDLL.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;

class EZ_ENGINEPLUGINJOLT_DLL ezJoltCollisionMeshContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltCollisionMeshContext, ezEngineProcessDocumentContext);

public:
  ezJoltCollisionMeshContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  const ezJoltMeshResourceHandle& GetMesh() const { return m_hMesh; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);

  ezGameObject* m_pMeshObject;
  ezJoltMeshResourceHandle m_hMesh;
};
