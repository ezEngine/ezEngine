#pragma once

#include <ToolsFoundation/Document/Document.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>

class ezTestDocument : public ezDocumentBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestDocument);

public:
  ezTestDocument(const char* szDocumentPath);
  ~ezTestDocument();

  virtual const char* GetDocumentTypeDisplayString() const override { return "Scene"; }

  virtual ezStatus InternalSaveDocument() override;

  void SelectionManagerEventHandler(const ezSelectionManager::Event& e);

  ezEditorGizmoHandle m_Gizmo;
};
