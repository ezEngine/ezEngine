#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class EZ_EDITORFRAMEWORK_DLL ezQtCompilerPreferencesWidget : public ezQtPropertyTypeWidget
{
  Q_OBJECT;

private Q_SLOTS:

  void on_compiler_preset_changed(int index);

public:
  explicit ezQtCompilerPreferencesWidget();
  virtual ~ezQtCompilerPreferencesWidget();

  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;

protected:
  QComboBox* m_pCompilerPreset;
};
