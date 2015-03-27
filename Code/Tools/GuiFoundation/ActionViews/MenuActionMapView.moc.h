#pragma once

#include <GuiFoundation/Basics.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <QMenu>

class QWidget;
class ezActionMap;
class QAction;
class ezQtProxy;


class EZ_GUIFOUNDATION_DLL ezMenuActionMapView : public QMenu
{
  Q_OBJECT
  EZ_DISALLOW_COPY_AND_ASSIGN(ezMenuActionMapView);
public:
  explicit ezMenuActionMapView(QWidget* parent = nullptr);
  ~ezMenuActionMapView();

  ezResult SetActionContext(const ezActionContext& context);

  static void AddDocumentObjectToMenu(ezHashTable<ezUuid, ezQtProxy*>& Proxies, ezActionContext& Context, ezActionMap* pActionMap, QMenu* pCurrentRoot, ezDocumentObjectBase* pObject);

private:
  void TreeEventHandler(const ezDocumentObjectTreeStructureEvent& e);
  void TreePropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e);

  void ClearView();
  void CreateView();
  
private:
  ezHashTable<ezUuid, ezQtProxy*> m_Proxies;

  ezActionContext m_Context;
  ezActionMap* m_pActionMap;
};
