#pragma once

#include <EditorPluginFileserve/EditorPluginFileserveDLL.h>
#include <QAbstractListModel>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>

class EZ_EDITORPLUGINFILESERVE_DLL ezQtFileserveAllFilesModel : public QAbstractListModel
{
  Q_OBJECT

public:
  ezQtFileserveAllFilesModel(QWidget* parent);

  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  void AddAccessedFile(const char* szFile);
  void UpdateView();

  void Clear();

  private Q_SLOTS:
  void UpdateViewSlot();

private:
  bool m_bTimerRunning = false;
  ezUInt32 m_uiAddedItems = 0;
  ezMap<ezString, ezUInt32> m_AllFiles;
  ezDeque<ezMap<ezString, ezUInt32>::Iterator> m_IndexedFiles;
};
