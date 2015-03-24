#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>



///
class EZ_GUIFOUNDATION_DLL ezActionManager
{
public:
  static ezActionDescriptorHandle RegisterAction(const ezActionDescriptor& desc);
  static bool UnregisterAction(ezActionDescriptorHandle hAction);
  static const ezActionDescriptor* GetActionDescriptor(ezActionDescriptorHandle hAction);
  static ezActionDescriptorHandle GetActionHandle(const ezHashedString& sCategoryPath, const char* szActionName);

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
  static ezMap<ezHashedString, CategoryData> s_CategoryPathToActions;
};

