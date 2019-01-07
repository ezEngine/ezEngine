#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Basics.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Containers/Deque.h>
#include <QAbstractItemModel>

/// \brief The Qt model that represents log output for a view
class EZ_GUIFOUNDATION_DLL ezQtLogModel : public QAbstractItemModel
{
  Q_OBJECT

public:


  ezQtLogModel(QObject* parent);
  void Clear();
  void SetLogLevel(ezLogMsgType::Enum LogLevel);
  void SetSearchText(const char* szText);
  void AddLogMsg(const ezLogEntry& msg);

  ezUInt32 GetVisibleItemCount() const { return m_VisibleMessages.GetCount(); }

public: //QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

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
};
