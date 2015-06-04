#pragma once

#include <ToolsFoundation/Document/Document.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>

class ezTestDocument : public ezDocumentBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestDocument);

public:
  ezTestDocument(const char* szDocumentPath);
  ~ezTestDocument();

  virtual const char* GetDocumentTypeDisplayString() const override { return "Scene"; }

  virtual ezStatus InternalSaveDocument() override;

  void SelectionManagerEventHandler(const ezSelectionManager::Event& e);

  ezTranslateGizmo m_TranslateGizmo;

protected:
  virtual void InitializeAfterLoading() override;

private:
  void TransformationGizmoEventHandler(const ezGizmoBase::BaseEvent& e);

  virtual ezDocumentInfo* CreateDocumentInfo() override { return EZ_DEFAULT_NEW(ezDocumentInfo); }
};
