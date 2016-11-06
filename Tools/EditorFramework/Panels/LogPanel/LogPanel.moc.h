#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <Tools/EditorFramework/ui_LogPanel.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Foundation/Logging/Log.h>

/// \brief The Qt model that represents log output for a view
class EZ_EDITORFRAMEWORK_DLL ezQtLogModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  struct LogMsg
  {
    ezString m_sMsg;
    ezString m_sTag;
    ezLogMsgType::Enum m_Type;
    ezUInt8 m_uiIndentation;
  };

  ezQtLogModel();
  void Clear();
  void SetLogLevel(ezLogMsgType::Enum LogLevel);
  void SetSearchText(const char* szText);
  void AddLogMsg(const LogMsg& msg);

  ezUInt32 GetVisibleItemCount() const { return m_VisibleMessages.GetCount(); }

public: //QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

private slots:
  /// \brief Adds queued messages from a different thread to the model.
  void ProcessNewMessages();

private:
  void Invalidate();
  bool IsFiltered(const LogMsg& lm) const;
  void UpdateVisibleEntries() const;

  ezLogMsgType::Enum m_LogLevel;
  ezString m_sSearchText;
  ezDeque<LogMsg> m_AllMessages;

  mutable bool m_bIsValid;
  mutable ezDeque<const LogMsg*> m_VisibleMessages;

  mutable ezMutex m_NewMessagesMutex;
  ezDeque<LogMsg> m_NewMessages;
};

/// \brief The application wide panel that shows the engine log output and the editor log output
class EZ_EDITORFRAMEWORK_DLL ezQtLogPanel : public ezQtApplicationPanel, public Ui_LogPanel
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtLogPanel);

public:
  ezQtLogPanel();
  ~ezQtLogPanel();

private slots:
  void on_ButtonClearEditorLog_clicked();
  void on_ButtonClearEngineLog_clicked();
  void on_ButtonClearSearch_clicked();
  void on_LineSearch_textChanged(const QString& text);
  void on_ComboFilter_currentIndexChanged(int index);

protected:
  virtual void ToolsProjectEventHandler(const ezToolsProjectEvent& e) override;

private:
  ezQtLogModel m_EngineLog;
  ezQtLogModel m_EditorLog;
  void ScrollToBottomIfAtEnd(QListView* pView, int iNumElements);

  void LogWriter(const ezLoggingEventData& e);
  void EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e);
};