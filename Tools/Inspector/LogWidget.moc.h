#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Tools/Inspector/ui_LogWidget.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>

class ezLogWidget : public QDockWidget, public Ui_LogWidget
{
public:
  Q_OBJECT

public:
  ezLogWidget(QWidget* parent = 0);

  void Log(const char* szFormat, ...);

  static ezLogWidget* s_pWidget;

private slots:
  virtual void on_ButtonClearLog_clicked();
  virtual void on_LineSearch_textChanged(QString sText);
  virtual void on_ComboLogLevel_currentIndexChanged(int iIndex);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  void UpdateLogList();

  struct LogMsg
  {
    ezString m_sMsg;
    ezString m_sTag;
    ezLogMsgType::Enum m_Type;
    ezUInt8 m_uiIndentation;
  };

  QListWidgetItem* CreateLogItem(const LogMsg& lm, ezInt32 iMessageIndex);
  bool IsFiltered(const LogMsg& lm);

  ezDeque<LogMsg> m_Messages;

  ezLogMsgType::Enum m_LogLevel;
  ezString m_sSearchText;
};


