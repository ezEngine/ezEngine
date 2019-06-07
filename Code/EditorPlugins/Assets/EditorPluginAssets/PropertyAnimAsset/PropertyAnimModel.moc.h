#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <QAbstractItemModel>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/DynamicArray.h>
#include <QIcon>

class ezPropertyAnimAssetDocument;
struct ezDocumentObjectPropertyEvent;
struct ezDocumentObjectStructureEvent;
class ezPropertyAnimationTrack;

struct ezQtPropertyAnimModelTreeEntry
{
  ezString m_sPathToItem;
  ezInt32 m_iParent = -1;
  ezUInt16 m_uiOwnRowIndex = 0;
  ezPropertyAnimationTrack* m_pTrack = nullptr;
  ezInt32 m_iTrackIdx = -1;
  ezString m_sDisplay;
  ezDynamicArray<ezInt32> m_Children;
  QIcon m_Icon;

  bool operator==(const ezQtPropertyAnimModelTreeEntry& rhs) const
  {
    return (m_iParent == rhs.m_iParent) && (m_uiOwnRowIndex == rhs.m_uiOwnRowIndex) && (m_pTrack == rhs.m_pTrack) &&
      (m_iTrackIdx == rhs.m_iTrackIdx) && (m_sDisplay == rhs.m_sDisplay) && (m_Children == rhs.m_Children);
  }

  bool operator!=(const ezQtPropertyAnimModelTreeEntry& rhs) const
  {
    return !(*this == rhs);
  }
};

class ezQtPropertyAnimModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  ezQtPropertyAnimModel(ezPropertyAnimAssetDocument* pDocument, QObject* pParent);
  ~ezQtPropertyAnimModel();

  enum UserRoles
  {
    TrackPtr = Qt::UserRole + 1,
    TreeItem = Qt::UserRole + 2,
    TrackIdx = Qt::UserRole + 3,
    Path = Qt::UserRole + 4,
  };

  const ezDeque<ezQtPropertyAnimModelTreeEntry>& GetAllEntries() const { return m_AllEntries[m_iInUse]; }

private Q_SLOTS:
  void onBuildMappingTriggered();

public: //QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:

  void DocumentStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void DocumentPropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void TriggerBuildMapping();
  void BuildMapping();
  void BuildMapping(ezInt32 iToUse);
  void BuildMapping(ezInt32 iToUse, ezInt32 iTrackIdx, ezPropertyAnimationTrack* pTrack, ezDynamicArray<ezInt32>& treeItems, ezInt32 iParentEntry, const char* szPath);

  bool m_bBuildMappingQueued = false;
  ezInt32 m_iInUse = 0;
  ezDynamicArray<ezInt32> m_TopLevelEntries[2];
  ezDeque<ezQtPropertyAnimModelTreeEntry> m_AllEntries[2];

  ezPropertyAnimAssetDocument* m_pAssetDoc = nullptr;
};
