#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Logging/LogEntry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QAbstractItemModel>

/// \brief The Qt model that represents log output for a view
class EZ_GUIFOUNDATION_DLL ezQtLogModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  ezQtLogModel(QObject* pParent);
  void Clear();
  void SetLogLevel(ezLogMsgType::Enum logLevel);
  void SetSearchText(const char* szText);
  void AddLogMsg(const ezLogEntry& msg);

  ezUInt32 GetVisibleItemCount() const { return m_VisibleMessages.GetCount(); }

  ezUInt32 GetNumErrors() const { return m_uiNumErrors; }
  ezUInt32 GetNumSeriousWarnings() const { return m_uiNumSeriousWarnings; }
  ezUInt32 GetNumWarnings() const { return m_uiNumWarnings; }

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int iRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

Q_SIGNALS:
  void NewErrorsOrWarnings(const char* szLatest, bool bError);

private Q_SLOTS:
  /// \brief Adds queued messages from a different thread to the model.
  void ProcessNewMessages();

private:
  void Invalidate();
  bool IsFiltered(const ezLogEntry& lm) const;
  void UpdateVisibleEntries() const;

  ezLogMsgType::Enum m_LogLevel;
  ezString m_sSearchText;
  ezDeque<ezLogEntry> m_AllMessages;

  mutable bool m_bIsValid;
  mutable ezDeque<const ezLogEntry*> m_VisibleMessages;
  mutable ezHybridArray<const ezLogEntry*, 16> m_BlockQueue;

  mutable ezMutex m_NewMessagesMutex;
  ezDeque<ezLogEntry> m_NewMessages;

  ezUInt32 m_uiNumErrors = 0;
  ezUInt32 m_uiNumSeriousWarnings = 0;
  ezUInt32 m_uiNumWarnings = 0;
};
