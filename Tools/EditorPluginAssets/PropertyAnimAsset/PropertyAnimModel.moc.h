#pragma once

#include <EditorFramework/Plugin.h>
#include <QAbstractItemModel>
#include <Foundation/Containers/Deque.h>

class ezPropertyAnimAssetDocument;
struct ezDocumentObjectPropertyEvent;
struct ezDocumentObjectStructureEvent;

class ezQtPropertyAnimModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  ezQtPropertyAnimModel(ezPropertyAnimAssetDocument* pDocument, QObject* pParent);
  ~ezQtPropertyAnimModel();

  enum UserRoles
  {
    TrackPtr = Qt::UserRole + 1,
  };

private slots:

public: //QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  //virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:

  struct TreeEntry
  {
    TreeEntry* m_pParent = nullptr;
    ezUInt16 m_uiOwnRowIndex = 0;
    ezPropertyAnimationTrack* m_pTrack = nullptr;
    ezString m_sDisplay;
    ezDynamicArray<TreeEntry*> m_Children;
  };

  void DocumentObjectEventHandler(const ezDocumentObjectPropertyEvent& e);
  void DocumentStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void BuildMapping();
  void BuildMapping(ezPropertyAnimationTrack* pTrack, ezDynamicArray<TreeEntry*>& treeItems, TreeEntry* pParentEntry, const char* szPath);

  ezDynamicArray<TreeEntry*> m_TopLevelEntries;
  ezDeque<TreeEntry> m_AllEntries;

  ezPropertyAnimAssetDocument* m_pAssetDoc = nullptr;
};
