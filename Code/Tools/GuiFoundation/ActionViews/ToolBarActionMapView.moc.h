#pragma once

#include <GuiFoundation/Basics.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <QToolBar>

class QWidget;
class ezActionMap;
class QAction;
class ezQtProxy;
class QMenu;

class EZ_GUIFOUNDATION_DLL ezToolBarActionMapView : public QToolBar
{
  Q_OBJECT
  EZ_DISALLOW_COPY_AND_ASSIGN(ezToolBarActionMapView);
public:
  explicit ezToolBarActionMapView(QWidget* parent = nullptr);
  ~ezToolBarActionMapView();

  ezResult SetActionContext(const ezActionContext& context);

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
