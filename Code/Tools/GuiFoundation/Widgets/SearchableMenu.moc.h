#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/Basics.h>
#include <QWidgetAction>

class ezQtSearchWidget;
class QTreeWidget;
class QTreeWidgetItem;
class ezQtTreeSearchFilterModel;
class QStandardItemModel;
class QTreeView;
class QStandardItem;

/// \brief Implements an item for insertion into a QMenu that shows a search bar and a hierarchical list of options.
///
/// Fill the searchable menu object with items (use slashes to indicate hierarchy) then use QMenu::addAction to insert it
/// into another QMenu.
/// Connect to MenuItemTriggered() to handle the item activation and also call QMenu::close() on the parent menu.
class EZ_GUIFOUNDATION_DLL ezQtSearchableMenu : public QWidgetAction
{
  Q_OBJECT
public:
  /// \brief The parent should usually be a QMenu into which this QWidgetAction is inserted as an action.
  ezQtSearchableMenu(QObject* parent);

  /// \brief Use slashes to separate sub-items.
  void AddItem(const char* szName, const QVariant& variant, QIcon icon = QIcon());

  /// \brief Returns the currently entered search text.
  QString GetSearchText() const;

  /// \brief Sets up the internal data model and ensures the menu's search bar gets input focus. Do this after adding the item to the parent menu.
  void Finalize(const QString& sSearchText);

Q_SIGNALS:
  /// \brief Signaled when an item is double clicked or otherwise selected for activation.
  void MenuItemTriggered(const QString& sName, const QVariant& variant);

  /// \brief Triggered whenever the search text is modified.
  void SearchTextChanged(const QString& text);

private Q_SLOTS:
  void OnItemActivated(const QModelIndex& index);
  void OnEnterPressed();
  void OnSpecialKeyPressed(Qt::Key key);
  void OnSearchChanged(const QString& text);

protected:
  virtual bool eventFilter(QObject*, QEvent*) override;

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
