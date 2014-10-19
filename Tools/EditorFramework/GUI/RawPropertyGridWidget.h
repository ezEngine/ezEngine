#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Containers/HybridArray.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <QWidget>
#include <QLayout>

class QGroupBox;
class ezDocumentBase;

class EZ_EDITORFRAMEWORK_DLL ezRawPropertyGridWidget : public QWidget
{
public:
  ezRawPropertyGridWidget(ezDocumentBase* pDocument, QWidget* pParent);
  ~ezRawPropertyGridWidget();

  void ClearSelection();
  void SetSelection(const ezDeque<const ezDocumentObjectBase*>&);

private:
  void BuildUI(const ezIReflectedTypeAccessor& et, const ezReflectedType* pType, ezPropertyPath& ParentPath, QLayout* pLayout, bool bEditorProp);
  void EditorPropertyChangedHandler(const ezPropertyEditorBaseWidget::EventData& ed);
  void ObjectPropertyChangedHandler(const ezPropertyEditorBaseWidget::EventData& ed);
  void SelectionEventHandler(const ezSelectionManager::Event& e);

  ezDeque<const ezDocumentObjectBase*> m_Selection;
  QVBoxLayout* m_pLayout;
  QWidget* m_pMainContent;
  QGroupBox* m_pGroups[2];
  QSpacerItem* m_pSpacer;
  ezDocumentBase* m_pDocument;
};


