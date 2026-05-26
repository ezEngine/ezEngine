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
  enum UserRoles
  {
    Link = Qt::UserRole + 1,
    LinkText = Qt::UserRole + 2,
    LinkTarget = Qt::UserRole + 3,
  };

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
  /// Wraps a log entry with optional embedded link data parsed from [[text|target]] syntax.
  struct ModelEntry
  {
    ezLogEntry m_Log;
    ezStringView m_sLink;       ///< Full [[text|target]] substring, empty if no link present.
    ezStringView m_sLinkText;   ///< Display text of the link.
    ezStringView m_sLinkTarget; ///< Link target (e.g. "asset:{guid}").
  };

  void Invalidate();
  bool IsFiltered(const ezLogEntry& lm) const;
  void UpdateVisibleEntries() const;
  void FindLink(ModelEntry& ref_entry) const;

  ezLogMsgType::Enum m_LogLevel;
  ezString m_sSearchText;
  ezDeque<ModelEntry> m_AllMessages;

  mutable bool m_bIsValid;
  mutable ezDeque<const ModelEntry*> m_VisibleMessages;
  mutable ezHybridArray<const ModelEntry*, 16> m_BlockQueue;

  mutable ezMutex m_NewMessagesMutex;
  ezDeque<ezLogEntry> m_NewMessages;
  bool m_bProcessingPending = false; ///< Prevents duplicate queued ProcessNewMessages invocations

  ezUInt32 m_uiNumErrors = 0;
  ezUInt32 m_uiNumSeriousWarnings = 0;
  ezUInt32 m_uiNumWarnings = 0;
};
