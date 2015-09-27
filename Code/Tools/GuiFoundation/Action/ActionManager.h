#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>

#define EZ_REGISTER_ACTION_0(ActionName, Label, Scope, CategoryName, ShortCut, ActionClass) \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
    [](const ezActionContext& context)->ezAction* { return EZ_DEFAULT_NEW(ActionClass, context, Label); }));

#define EZ_REGISTER_ACTION_1(ActionName, Label, Scope, CategoryName, ShortCut, ActionClass, Param1) \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
    [](const ezActionContext& context)->ezAction* { return EZ_DEFAULT_NEW(ActionClass, context, Label, Param1); }));

#define EZ_REGISTER_ACTION_2(ActionName, Label, Scope, CategoryName, ShortCut, ActionClass, Param1, Param2) \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
    [](const ezActionContext& context)->ezAction* { return EZ_DEFAULT_NEW(ActionClass, context, Label, Param1, Param2); }));

#define EZ_REGISTER_LRU_MENU(ActionName, Label, ActionClass, IconPath) \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Menu, ezActionScope::Default, ActionName, "", "", \
    [](const ezActionContext& context)->ezAction* { return EZ_DEFAULT_NEW(ActionClass, context, Label, IconPath); }));

#define EZ_REGISTER_MENU(ActionName, Label) \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Menu, ezActionScope::Default, ActionName, "", "", \
    [](const ezActionContext& context)->ezAction*{ return EZ_DEFAULT_NEW(ezMenuAction, context, Label, ""); }));

#define EZ_REGISTER_MENU_WITH_ICON(ActionName, Label, IconPath) \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Menu, ezActionScope::Default, ActionName, "", "", \
    [](const ezActionContext& context)->ezAction*{ return EZ_DEFAULT_NEW(ezMenuAction, context, Label, IconPath); }));

#define EZ_REGISTER_CATEGORY(CategoryName) \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Category, ezActionScope::Default, CategoryName, "", "", \
    [](const ezActionContext& context)->ezAction*{ return EZ_DEFAULT_NEW(ezCategoryAction, context); }));

///
class EZ_GUIFOUNDATION_DLL ezActionManager
{
public:
  static ezActionDescriptorHandle RegisterAction(const ezActionDescriptor& desc);
  static bool UnregisterAction(ezActionDescriptorHandle& hAction);
  static const ezActionDescriptor* GetActionDescriptor(ezActionDescriptorHandle hAction);
  static ezActionDescriptorHandle GetActionHandle(const char* szCategoryPath, const char* szActionName);

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
};

