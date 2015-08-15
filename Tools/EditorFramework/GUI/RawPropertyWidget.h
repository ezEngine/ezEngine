#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <QWidget>
#include <QLayout>

class EZ_EDITORFRAMEWORK_DLL ezRawPropertyWidget : public QWidget
{
public:
  ezRawPropertyWidget(QWidget* pParent, const ezHybridArray<ezPropertyEditorBaseWidget::Selection, 8>& items, bool bEditorProperty);

  ezEvent<const ezPropertyEditorBaseWidget::Event&> m_PropertyChanged;

  void ChangePropertyValue(const ezString& sPropertyPath, const ezVariant& value);

private:
  void BuildUI(const ezHybridArray<ezPropertyEditorBaseWidget::Selection, 8>& items, bool bEditorProperty, const ezRTTI* pType, ezPropertyPath& ParentPath, QLayout* pLayout);

  void PropertyChangedHandler(const ezPropertyEditorBaseWidget::Event& ed);

  QVBoxLayout* m_pLayout;

  ezMap<ezString, ezPropertyEditorBaseWidget*> m_PropertyWidgets;
};


