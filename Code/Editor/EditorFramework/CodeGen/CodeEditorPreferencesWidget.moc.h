#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class EZ_EDITORFRAMEWORK_DLL ezQtCodeEditorPreferencesWidget : public ezQtPropertyTypeWidget
{
  Q_OBJECT;

private Q_SLOTS:
  void on_code_editor_changed(int index);

public:
  explicit ezQtCodeEditorPreferencesWidget();
  virtual ~ezQtCodeEditorPreferencesWidget();

  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;

protected:
  QComboBox* m_pCodeEditor;
};
