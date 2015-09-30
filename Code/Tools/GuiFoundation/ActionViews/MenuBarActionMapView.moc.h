#pragma once

#include <GuiFoundation/Basics.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <QMenuBar>
#include <QSharedPointer>

class QWidget;
class ezActionMap;
class QAction;
class ezQtProxy;

class EZ_GUIFOUNDATION_DLL ezMenuBarActionMapView : public QMenuBar
{
  Q_OBJECT
  EZ_DISALLOW_COPY_AND_ASSIGN(ezMenuBarActionMapView);
public:
  explicit ezMenuBarActionMapView(QWidget* parent);
  ~ezMenuBarActionMapView();

  void SetActionContext(const ezActionContext& context);

private:
  void TreeEventHandler(const ezDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  void ClearView();
  void CreateView();

private:
  ezHashTable<ezUuid, QSharedPointer<ezQtProxy>> m_Proxies;

  ezActionContext m_Context;
  ezActionMap* m_pActionMap;
};
