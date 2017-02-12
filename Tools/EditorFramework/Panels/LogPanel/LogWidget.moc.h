#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/Plugin.h>
#include <Tools/EditorFramework/ui_LogWidget.h>
#include <QWidget>

class ezQtLogModel;

/// \brief The application wide panel that shows the engine log output and the editor log output
class EZ_EDITORFRAMEWORK_DLL ezQtLogWidget : public QWidget, public Ui_LogWidget
{
  Q_OBJECT

public:
  ezQtLogWidget(QWidget* parent);
  ~ezQtLogWidget();

  ezQtLogModel* GetLog();
  ezQtSearchWidget* GetSearchWidget();

  virtual bool eventFilter(QObject* pObject, QEvent* pEvent) override;

private slots:
  void on_ButtonClearLog_clicked();
  void on_Search_textChanged(const QString& text);
  void on_ComboFilter_currentIndexChanged(int index);

private:
  ezQtLogModel* m_pLog;
  void ScrollToBottomIfAtEnd(int iNumElements);
};
