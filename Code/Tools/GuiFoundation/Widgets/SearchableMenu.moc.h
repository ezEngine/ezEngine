#pragma once

#include <GuiFoundation/Basics.h>
#include <QWidgetAction>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

class ezQtSearchWidget;
class QTreeWidget;
class QTreeWidgetItem;
class ezQtTreeSearchFilterModel;
class QStandardItemModel;
class QTreeView;
class QStandardItem;

class EZ_GUIFOUNDATION_DLL ezQtSearchableMenu : public QWidgetAction
{
  Q_OBJECT
public:
  ezQtSearchableMenu(QObject* parent);

  void AddItem(const char* szName, const QVariant& variant);
  QString GetSearchText() const;

  void Finalize(const QString& sSearchText);

signals:
  void MenuItemTriggered(const QString& sName, const QVariant& variant);

  private slots:
  void OnItemActivated(const QModelIndex& index);
  void OnEnterPressed();
  void OnSpecialKeyPressed(Qt::Key key);
  void OnSearchChanged(const QString& text);

protected:
  virtual bool eventFilter(QObject *, QEvent *) override;

private:
  QStandardItem* CreateCategoryMenu(const char* szCategory);
  bool SelectFirstLeaf(QModelIndex parent);

  QWidget* m_pGroup;
  ezQtSearchWidget* m_pSearch;
  ezQtTreeSearchFilterModel* m_pFilterModel;
  QTreeView* m_pTreeView;
  QStandardItemModel* m_pItemModel;
  ezMap<ezString, QStandardItem*> m_Hierarchy;

};


