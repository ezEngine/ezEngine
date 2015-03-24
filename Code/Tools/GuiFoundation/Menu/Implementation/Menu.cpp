#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Menu/Menu.moc.h>

ezRttiMappedObjectFactory<QMenu> ezMenuActionMapView::s_MenuFactory;
ezRttiMappedObjectFactory<QAction> ezMenuActionMapView::s_CategoryFactory;
ezRttiMappedObjectFactory<QAction> ezMenuActionMapView::s_ActionFactory;

ezMenuActionMapView::ezMenuActionMapView(QWidget* parent)
{
}

ezMenuActionMapView::~ezMenuActionMapView()
{
}
