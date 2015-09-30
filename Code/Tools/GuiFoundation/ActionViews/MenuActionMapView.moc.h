#pragma once

#include <GuiFoundation/Basics.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <QMenu>
#include <QSharedPointer>

class QWidget;
class ezActionMap;
class QAction;
class ezQtProxy;


class EZ_GUIFOUNDATION_DLL ezMenuActionMapView : public QMenu
{
  Q_OBJECT
  EZ_DISALLOW_COPY_AND_ASSIGN(ezMenuActionMapView);
public:
  explicit ezMenuActionMapView(QWidget* parent);
  ~ezMenuActionMapView();

  void SetActionContext(const ezActionContext& context);

  static void AddDocumentObjectToMenu(ezHashTable<ezUuid, QSharedPointer<ezQtProxy>>& Proxies, ezActionContext& Context, ezActionMap* pActionMap, QMenu* pCurrentRoot, const ezActionMap::TreeNode* pObject);

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
