#pragma once

#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QMenuBar>
#include <QSharedPointer>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

class QWidget;
class ezActionMap;
class QAction;
class ezQtProxy;

class EZ_GUIFOUNDATION_DLL ezQtMenuBarActionMapView : public QMenuBar
{
  Q_OBJECT
  EZ_DISALLOW_COPY_AND_ASSIGN(ezQtMenuBarActionMapView);

public:
  explicit ezQtMenuBarActionMapView(QWidget* parent);
  ~ezQtMenuBarActionMapView();

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
