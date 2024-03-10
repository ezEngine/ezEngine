#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <QLineEdit>

class ezQtFileLineEdit;

class EZ_EDITORFRAMEWORK_DLL ezQtFilePropertyWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtFilePropertyWidget();
  bool IsValidFileReference(ezStringView sFile) const;

private Q_SLOTS:
  void on_BrowseFile_clicked();

protected slots:
  void on_TextFinished_triggered();
  void on_TextChanged_triggered(const QString& value);
  void OnOpenExplorer();
  void OnCustomAction();
  void OnOpenFile();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  QHBoxLayout* m_pLayout = nullptr;
  ezQtFileLineEdit* m_pWidget = nullptr;
  QToolButton* m_pButton = nullptr;
};
