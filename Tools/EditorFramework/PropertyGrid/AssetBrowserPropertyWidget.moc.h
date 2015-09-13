#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

/// *** Asset Browser ***

class EZ_EDITORFRAMEWORK_DLL ezAssetBrowserPropertyWidget : public ezPropertyEditorLineEditWidget
{
  Q_OBJECT

public:
  ezAssetBrowserPropertyWidget();

  private slots:
  void on_BrowseFile_clicked();

protected:
  virtual void OnInit() override;

protected:
  QToolButton* m_pButton;
};
