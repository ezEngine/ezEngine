#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/ActionManager.h>

const ezActionDescriptor* ezActionHandle::GetDescriptor() const
{
  return ezActionManager::GetActionDescriptor(*this);
}

ezActionDescriptor::ezActionDescriptor(ActionType::Enum type, ActionScope::Enum scope, const char* szName, ezHashedString sCategoryPath,
                                       CreateActionFunc createAction, DeleteActionFunc deleteAction)
  : m_Type(type)
  , m_Scope(scope)
  , m_sActionName(szName)
  , m_sCategoryPath(sCategoryPath)
  , m_CreateAction(createAction)
  , m_DeleteAction(deleteAction)
{
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAction, ezReflectedClass, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();
