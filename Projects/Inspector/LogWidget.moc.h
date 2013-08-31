#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Projects/Inspector/ui_LogWidget.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>

class ezLogWidget : public QDockWidget, public Ui_LogWidget
{
public:
  Q_OBJECT

public:
  ezLogWidget(QWidget* parent = 0);

  static ezLogWidget* s_pWidget;

private slots:
  virtual void on_ButtonClearLog_clicked();
  virtual void on_LineSearch_textChanged(QString sText);
  virtual void on_ComboLogLevel_currentIndexChanged(int iIndex);

public:
  static void ProcessTelemetry_Log(void* pPassThrough);

  void ResetStats();
  void UpdateStats();

private:
  void UpdateLogList();

  struct LogMsg
  {
    ezString m_sMsg;
    ezString m_sTag;
    ezLog::EventType::Enum m_Type;
    ezUInt8 m_uiIndentation;
  };

  QListWidgetItem* CreateLogItem(const LogMsg& lm, ezInt32 iMessageIndex);
  bool IsFiltered(const LogMsg& lm);

  ezDeque<LogMsg> m_Messages;

  ezLog::EventType::Enum m_LogLevel;
  ezString m_sSearchText;
};


