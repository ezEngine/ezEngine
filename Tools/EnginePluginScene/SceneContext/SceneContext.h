#pragma once

#include <EnginePluginScene/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>

class ezObjectSelectionMsgToEngine;

class EZ_ENGINEPLUGINSCENE_DLL ezSceneContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneContext, ezEngineProcessDocumentContext);

public:

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  const ezDeque<ezGameObjectHandle>& GetSelection() const { return m_Selection; }
  const ezDeque<ezGameObjectHandle>& GetSelectionWithChildren() const { return m_SelectionWithChildren; }

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() {}

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  void HandleSelectionMsg(const ezObjectSelectionMsgToEngine* pMsg);
  void ComputeHierarchyBounds(ezGameObject* pObj, ezBoundingBoxSphere& bounds);
  void InsertSelectedChildren(const ezGameObject* pObject);

  ezDeque<ezGameObjectHandle> m_Selection;
  ezDeque<ezGameObjectHandle> m_SelectionWithChildren;
  ezSet<ezGameObjectHandle> m_SelectionWithChildrenSet;
};


