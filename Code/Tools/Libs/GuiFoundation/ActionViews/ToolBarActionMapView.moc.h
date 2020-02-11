#pragma once

#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QSharedPointer>
#include <QToolBar>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

class QWidget;
class ezActionMap;
class QAction;
class ezQtProxy;
class QMenu;

class EZ_GUIFOUNDATION_DLL ezQtToolBarActionMapView : public QToolBar
{
  Q_OBJECT
  EZ_DISALLOW_COPY_AND_ASSIGN(ezQtToolBarActionMapView);

public:
  explicit ezQtToolBarActionMapView(QString title, QWidget* parent);
  ~ezQtToolBarActionMapView();

  void SetActionContext(const ezActionContext& context);

  virtual void setVisible(bool visible) override;

private:
  void TreeEventHandler(const ezDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  void ClearView();
  void CreateView();
  void CreateView(const ezActionMap::TreeNode* pRoot);

private:
  ezHashTable<ezUuid, QSharedPointer<ezQtProxy>> m_Proxies;

  ezActionContext m_Context;
  ezActionMap* m_pActionMap;
};
