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


class EZ_GUIFOUNDATION_DLL ezQtMenuActionMapView : public QMenu
{
  Q_OBJECT
  EZ_DISALLOW_COPY_AND_ASSIGN(ezQtMenuActionMapView);
public:
  explicit ezQtMenuActionMapView(QWidget* parent);
  ~ezQtMenuActionMapView();

  void SetActionContext(const ezActionContext& context);

  static void AddDocumentObjectToMenu(ezHashTable<ezUuid, QSharedPointer<ezQtProxy>>& Proxies, ezActionContext& Context, ezActionMap* pActionMap, QMenu* pCurrentRoot, const ezActionMap::TreeNode* pObject);

private:
  void ClearView();
  void CreateView();
  
private:
  ezHashTable<ezUuid, QSharedPointer<ezQtProxy>> m_Proxies;

  ezActionContext m_Context;
  ezActionMap* m_pActionMap;
};
