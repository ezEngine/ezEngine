#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Containers/HybridArray.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <QWidget>
#include <QLayout>

class QGroupBox;

class EZ_EDITORFRAMEWORK_DLL ezRawPropertyGridWidget : public QWidget
{
public:
  ezRawPropertyGridWidget(QWidget* pParent);
  ~ezRawPropertyGridWidget();

  void SetSelection(const ezHybridArray<ezDocumentObjectBase*, 32>& selection);

private:
  void BuildUI(const ezIReflectedTypeAccessor& et, const ezReflectedType* pType, ezPropertyPath& ParentPath, QLayout* pLayout, bool bEditorProp);
  void EditorPropertyChangedHandler(const ezPropertyEditorBaseWidget::EventData& ed);
  void ObjectPropertyChangedHandler(const ezPropertyEditorBaseWidget::EventData& ed);

  ezHybridArray<ezDocumentObjectBase*, 32> m_Selection;
  QVBoxLayout* m_pLayout;
  QGroupBox* m_pGroups[2];
};


