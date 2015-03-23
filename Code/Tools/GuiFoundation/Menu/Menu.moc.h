#pragma once

#include <GuiFoundation/Basics.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <QMenu>

class QWidget;
class ezActionMap;
class QAction;

class EZ_GUIFOUNDATION_DLL ezMenuActionMapView : public QMenu
{
  Q_OBJECT
  EZ_DISALLOW_COPY_AND_ASSIGN(ezMenuActionMapView);
public:
  explicit ezMenuActionMapView(QWidget* parent = nullptr);
  ~ezMenuActionMapView();

  ezResult SetActionMap(const ezHashedString& sMapping);

private:
  void TreeEventHandler(const ezDocumentObjectTreeStructureEvent& e);
  void TreePropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e);

public:
  static ezRttiMappedObjectFactory<QMenu> s_MenuFactory;
  static ezRttiMappedObjectFactory<QAction> s_CategoryFactory;
  static ezRttiMappedObjectFactory<QAction> s_ActionFactory;

private:
  ezHashTable<ezUuid, QMenu*> m_Menus;
  ezHashTable<ezUuid, QAction*> m_Actions;

  ezHashedString m_sMapping;
  ezActionMap* m_pActionMap;

};
