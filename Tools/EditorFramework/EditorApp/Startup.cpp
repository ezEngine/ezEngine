#include <PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <PropertyGrid/FileBrowserPropertyWidget.moc.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, EditorFrameworkMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "GuiFoundation",
    "PropertyGrid"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezProjectActions::RegisterActions();
    ezAssetActions::RegisterActions();
    ezViewActions::RegisterActions();

    ezActionMapManager::RegisterActionMap("SettingsTabMenuBar");
    ezProjectActions::MapActions("SettingsTabMenuBar");
    ezStandardMenus::MapActions("SettingsTabMenuBar", ezStandardMenuTypes::Panels);
    
    ezActionMapManager::RegisterActionMap("AssetBrowserToolBar");
    ezAssetActions::MapActions("AssetBrowserToolBar", false);

    ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtFilePropertyWidget(); });
    ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezAssetBrowserAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtAssetPropertyWidget(); });
    ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezDynamicEnumAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtDynamicEnumPropertyWidget(); });
  }

  ON_CORE_SHUTDOWN
  {
    ezProjectActions::UnregisterActions();
    ezAssetActions::UnregisterActions();
    ezViewActions::UnregisterActions();

    ezPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>());
    ezPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezAssetBrowserAttribute>());
    ezPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezDynamicEnumAttribute>());
  }

EZ_END_SUBSYSTEM_DECLARATION


