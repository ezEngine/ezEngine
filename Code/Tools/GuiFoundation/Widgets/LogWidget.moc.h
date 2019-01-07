#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Basics.h>
#include <Code/Tools/GuiFoundation/ui_LogWidget.h>
#include <Foundation/Logging/Log.h>
#include <QWidget>

class ezQtLogModel;
class ezQtSearchWidget;

/// \brief The application wide panel that shows the engine log output and the editor log output
class EZ_GUIFOUNDATION_DLL ezQtLogWidget : public QWidget, public Ui_LogWidget
{
  Q_OBJECT

public:
  ezQtLogWidget(QWidget* parent);
  ~ezQtLogWidget();

  ezQtLogModel* GetLog();
  ezQtSearchWidget* GetSearchWidget();
  void SetLogLevel(ezLogMsgType::Enum logLevel);
  ezLogMsgType::Enum GetLogLevel() const;

  virtual bool eventFilter(QObject* pObject, QEvent* pEvent) override;

private Q_SLOTS:
  void on_ButtonClearLog_clicked();
  void on_Search_textChanged(const QString& text);
  void on_ComboFilter_currentIndexChanged(int index);

private:
  ezQtLogModel* m_pLog;
  void ScrollToBottomIfAtEnd(int iNumElements);
};
