#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <QLineEdit>

class EZ_EDITORFRAMEWORK_DLL ezQtFilePropertyWidget : public ezQtStandardPropertyWidget
{
  Q_OBJECT

public:
  ezQtFilePropertyWidget();

private slots:
  void on_BrowseFile_clicked();

protected slots:
  void on_TextFinished_triggered();
  void on_TextChanged_triggered(const QString& value);
  void on_customContextMenuRequested(const QPoint& pt);
  void OnOpenExplorer();
  void OnOpenFile();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const ezVariant& value) override;

protected:
  QHBoxLayout* m_pLayout;
  QLineEdit* m_pWidget;
  QToolButton* m_pButton;
};
