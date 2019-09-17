#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>

#define EZ_REGISTER_ACTION_0(ActionName, Scope, CategoryName, ShortCut, ActionClass)                                  \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
    [](const ezActionContext& context) -> ezAction* { return EZ_DEFAULT_NEW(ActionClass, context, ActionName); }));

#define EZ_REGISTER_ACTION_1(ActionName, Scope, CategoryName, ShortCut, ActionClass, Param1)                          \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
    [](const ezActionContext& context) -> ezAction* { return EZ_DEFAULT_NEW(ActionClass, context, ActionName, Param1); }));

#define EZ_REGISTER_ACTION_2(ActionName, Scope, CategoryName, ShortCut, ActionClass, Param1, Param2)                  \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
    [](const ezActionContext& context) -> ezAction* { return EZ_DEFAULT_NEW(ActionClass, context, ActionName, Param1, Param2); }));

#define EZ_REGISTER_DYNAMIC_MENU(ActionName, ActionClass, IconPath)                                                  \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Menu, ezActionScope::Default, ActionName, "", "", \
    [](const ezActionContext& context) -> ezAction* { return EZ_DEFAULT_NEW(ActionClass, context, ActionName, IconPath); }));

#define EZ_REGISTER_ACTION_AND_DYNAMIC_MENU_1(ActionName, Scope, CategoryName, ShortCut, ActionClass, Param1)                \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::ActionAndMenu, Scope, ActionName, CategoryName, ShortCut, \
    [](const ezActionContext& context) -> ezAction* { return EZ_DEFAULT_NEW(ActionClass, context, ActionName, Param1); }));

#define EZ_REGISTER_MENU(ActionName)                                                                                 \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Menu, ezActionScope::Default, ActionName, "", "", \
    [](const ezActionContext& context) -> ezAction* { return EZ_DEFAULT_NEW(ezMenuAction, context, ActionName, ""); }));

#define EZ_REGISTER_MENU_WITH_ICON(ActionName, IconPath)                                                             \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Menu, ezActionScope::Default, ActionName, "", "", \
    [](const ezActionContext& context) -> ezAction* { return EZ_DEFAULT_NEW(ezMenuAction, context, ActionName, IconPath); }));

#define EZ_REGISTER_CATEGORY(CategoryName)                                                                                 \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Category, ezActionScope::Default, CategoryName, "", "", \
    [](const ezActionContext& context) -> ezAction* { return EZ_DEFAULT_NEW(ezCategoryAction, context); }));

///
class EZ_GUIFOUNDATION_DLL ezActionManager
{
public:
  static ezActionDescriptorHandle RegisterAction(const ezActionDescriptor& desc);
  static bool UnregisterAction(ezActionDescriptorHandle& hAction);
  static const ezActionDescriptor* GetActionDescriptor(ezActionDescriptorHandle hAction);
  static ezActionDescriptorHandle GetActionHandle(const char* szCategory, const char* szActionName);

  /// \brief Searches all action categories for the given action name. Returns the category name in which the action name was found, or an empty string.
  static ezString FindActionCategory(const char* szActionName);

  /// \brief Quick way to execute an action from code
  ///
  /// The use case is mostly for unit tests, which need to execute actions directly and without a link dependency on
  /// the code that registered the action.
  ///
  /// \param szCategory The category of the action, ie. under which name the action appears in the Shortcut binding dialog.
  ///        For example "Scene", "Scene - Cameras", "Scene - Selection", "Assets" etc.
  ///        This parameter may be nullptr in which case FindActionCategory(szActionName) is used to try to detect the category automatically.
  /// \param szActionName The name (not mapped path) under which the action was registered.
  ///        For example "Selection.Copy", "Prefabs.ConvertToEngine", "Scene.Camera.SnapObjectToCamera"
  /// \param context The context in which to execute the action. Depending on the ezActionScope of the target action,
  ///        some members are optional. E.g. for document actions, only the m_pDocument member must be specified.
  /// \param value Optional value passed through to the ezAction::Execute() call. Some actions use it, most don't.
  /// \return Returns failure in case the action could not be found.
  static ezResult ExecuteAction(const char* szCategory, const char* szActionName, const ezActionContext& context, const ezVariant& value = ezVariant());

  static void SaveShortcutAssignment();
  static void LoadShortcutAssignment();

  static const ezIdTable<ezActionId, ezActionDescriptor*>::ConstIterator GetActionIterator();

  struct Event
  {
    enum class Type
    {
      ActionAdded,
      ActionRemoved
    };

    Type m_Type;
    const ezActionDescriptor* m_pDesc;
    ezActionDescriptorHandle m_Handle;
  };

  static ezEvent<const Event&> s_Events;

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionManager);

  static void Startup();
  static void Shutdown();
  static ezActionDescriptor* CreateActionDesc(const ezActionDescriptor& desc);
  static void DeleteActionDesc(ezActionDescriptor* pDesc);

  struct CategoryData
  {
    ezSet<ezActionDescriptorHandle> m_Actions;
    ezHashTable<const char*, ezActionDescriptorHandle> m_ActionNameToHandle;
  };

private:
  static ezIdTable<ezActionId, ezActionDescriptor*> s_ActionTable;
  static ezMap<ezString, CategoryData> s_CategoryPathToActions;
  static ezMap<ezString, ezString> s_ShortcutOverride;
};
