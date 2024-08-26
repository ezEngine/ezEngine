#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_LogWidget.h>
#include <QWidget>

class ezQtLogModel;
class ezQtSearchWidget;

/// \brief The application wide panel that shows the engine log output and the editor log output
class EZ_GUIFOUNDATION_DLL ezQtLogWidget : public QWidget, public Ui_LogWidget
{
  Q_OBJECT

public:
  ezQtLogWidget(QWidget* pParent);
  ~ezQtLogWidget();

  void ShowControls(bool bShow);

  ezQtLogModel* GetLog();
  ezQtSearchWidget* GetSearchWidget();
  void SetLogLevel(ezLogMsgType::Enum logLevel);
  ezLogMsgType::Enum GetLogLevel() const;

  virtual bool eventFilter(QObject* pObject, QEvent* pEvent) override;

  using LogItemContextActionCallback = ezDelegate<void(const ezStringView& sLogText)>;
  static bool AddLogItemContextActionCallback(const ezStringView& sName, const LogItemContextActionCallback& logCallback);
  static bool RemoveLogItemContextActionCallback(const ezStringView& sName);

private Q_SLOTS:
  void on_ButtonClearLog_clicked();
  void on_Search_textChanged(const QString& text);
  void on_ComboFilter_currentIndexChanged(int index);
  void OnItemDoubleClicked(QModelIndex idx);

private:
  ezQtLogModel* m_pLog;
  void ScrollToBottomIfAtEnd(int iNumElements);

  /// \brief List of callbacks invoked when the user double clicks a log message
  static ezMap<ezString, LogItemContextActionCallback> s_LogCallbacks;
};
