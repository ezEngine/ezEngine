#include <GuiFoundation/PCH.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>

ezMenuBarActionMapView::ezMenuBarActionMapView(QWidget* parent) : QMenuBar(parent)
{
}

ezMenuBarActionMapView::~ezMenuBarActionMapView()
{
  ClearView();
}

void ezMenuBarActionMapView::SetActionContext(const ezActionContext& context)
{
  auto pMap = ezActionMapManager::GetActionMap(context.m_sMapping);

   EZ_ASSERT_DEV(pMap != nullptr, "The given mapping '%s' does not exist", context.m_sMapping.GetData());

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();
}

void ezMenuBarActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void ezMenuBarActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->GetRootObject();

  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = m_pActionMap->GetDescriptor(pChild);

    QSharedPointer<ezQtProxy> pProxy = ezQtProxy::GetProxy(m_Context, pDesc->m_hAction);
    m_Proxies[pChild->GetGuid()] = pProxy;

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
    case ezActionType::Action:
      {
        EZ_REPORT_FAILURE("Cannot map actions in a menubar view!");
      }
      break;

    case ezActionType::Category:
      {
        EZ_REPORT_FAILURE("Cannot map category in a menubar view!");
      }
      break;

    case ezActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<ezQtMenuProxy*>(pProxy.data())->GetQMenu();
        addMenu(pQtMenu);
        ezMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}
