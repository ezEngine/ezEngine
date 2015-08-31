#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/BaseActions.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezNamedAction, ezAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCategoryAction, ezAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMenuAction, ezNamedAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLRUMenuAction, ezMenuAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezButtonAction, ezNamedAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezButtonAction::ezButtonAction(const ezActionContext& context, const char* szName, bool bCheckable, const char* szIconPath) : ezNamedAction(context, szName, szIconPath) 
{
  m_bCheckable = false;
  m_bChecked = false;
  m_bEnabled = true;
  m_bVisible = true;
}