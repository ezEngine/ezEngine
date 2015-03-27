#pragma once

#include <GuiFoundation/Basics.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <QMenuBar>

class QWidget;
class ezActionMap;
class QAction;
class ezQtProxy;

class EZ_GUIFOUNDATION_DLL ezMenuBarActionMapView : public QMenuBar
{
  Q_OBJECT
  EZ_DISALLOW_COPY_AND_ASSIGN(ezMenuBarActionMapView);
public:
  explicit ezMenuBarActionMapView(QWidget* parent = nullptr);
  ~ezMenuBarActionMapView();

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
