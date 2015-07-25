#pragma once

#include <GuiFoundation/Basics.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <QToolBar>
#include <QSharedPointer>

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
  explicit ezToolBarActionMapView(QWidget* parent, QWidget* pActionParent = nullptr);
  ~ezToolBarActionMapView();

  void SetActionContext(const ezActionContext& context);

private:
  void TreeEventHandler(const ezDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  void ClearView();
  void CreateView();
  void CreateView(ezDocumentObjectBase* pRoot);

private:
  ezHashTable<ezUuid, QSharedPointer<ezQtProxy>> m_Proxies;

  ezActionContext m_Context;
  ezActionMap* m_pActionMap;
  QWidget* m_pActionParent;
};
