#pragma once

#include <EnginePluginPhysX/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;
class ezGameState;

class EZ_ENGINEPLUGINPHYSX_DLL ezCollisionMeshContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollisionMeshContext, ezEngineProcessDocumentContext);

public:
  ezCollisionMeshContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);

  ezGameObject* m_pMeshObject;
  ezPxMeshResourceHandle m_hMesh;
};


