#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/ActionManager.h>

const ezActionDescriptor* ezActionDescriptorHandle::GetDescriptor() const
{
  return ezActionManager::GetActionDescriptor(*this);
}

ezActionDescriptor::ezActionDescriptor(ezActionType::Enum type, ezActionScope::Enum scope, const char* szName, const char* szCategoryPath, const char* szShortcut,
                                       CreateActionFunc createAction, DeleteActionFunc deleteAction)
  : m_Type(type)
  , m_Scope(scope)
  , m_sActionName(szName)
  , m_sCategoryPath(szCategoryPath)
  , m_sShortcut(szShortcut)
  , m_CreateAction(createAction)
  , m_DeleteAction(deleteAction)
{
}

ezAction* ezActionDescriptor::CreateAction(const ezActionContext& context) const
{
  EZ_ASSERT_DEV(!m_Handle.IsInvalidated(), "Handle invalid!");
  auto pAction = m_CreateAction(context);
  pAction->m_DescriptorHandle = m_Handle;
  return pAction;
}

void ezActionDescriptor::DeleteAction(ezAction* pAction) const
{
  if (m_DeleteAction == nullptr)
  {
    EZ_DEFAULT_DELETE(pAction);
  }
  else
    m_DeleteAction(pAction);
}

void ezAction::TriggerUpdate()
{
  m_StatusUpdateEvent.Broadcast(this);
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAction, ezReflectedClass, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();
