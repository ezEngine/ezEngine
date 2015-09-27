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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEnumerationMenuAction, ezLRUMenuAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezButtonAction, ezNamedAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezEnumerationMenuAction::ezEnumerationMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath) : ezLRUMenuAction(context, szName, szIconPath)
{
  m_pEnumerationType = nullptr;
}

void ezEnumerationMenuAction::InitEnumerationType(const ezRTTI* pEnumerationType)
{
  m_pEnumerationType = pEnumerationType;
  
}

void ezEnumerationMenuAction::GetEntries(ezHybridArray<ezLRUMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();
  out_Entries.Reserve(m_pEnumerationType->GetProperties().GetCount() - 1);
  ezInt64 iCurrentValue = ezReflectionUtils::MakeEnumerationValid(m_pEnumerationType, GetValue());

  ezStringBuilder sTemp;

  for (auto pProp : m_pEnumerationType->GetProperties().GetSubArray(1))
  {
    if (pProp->GetCategory() == ezPropertyCategory::Constant)
    {
      ezInt64 iValue = static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<ezInt64>();
      ezLRUMenuAction::Item item;

      // If this is a qualified C++ name, skip everything before the last colon
      sTemp = pProp->GetPropertyName();
      const char* szColon = sTemp.FindLastSubString(":");
      if (szColon != nullptr)
        item.m_sDisplay = szColon + 1;
      else
        item.m_sDisplay = sTemp;

      item.m_UserValue = iValue;
      if (m_pEnumerationType->IsDerivedFrom<ezEnumBase>())
      {
        item.m_CheckState = (iCurrentValue == iValue) ? ezLRUMenuAction::Item::CheckMark::Checked : ezLRUMenuAction::Item::CheckMark::Unchecked;
      }
      else if (m_pEnumerationType->IsDerivedFrom<ezBitflagsBase>())
      {
        item.m_CheckState = ((iCurrentValue & iValue) != 0) ? ezLRUMenuAction::Item::CheckMark::Checked : ezLRUMenuAction::Item::CheckMark::Unchecked;
      }
      
      out_Entries.PushBack(item);
    }
  }
}

ezButtonAction::ezButtonAction(const ezActionContext& context, const char* szName, bool bCheckable, const char* szIconPath) : ezNamedAction(context, szName, szIconPath) 
{
  m_bCheckable = false;
  m_bChecked = false;
  m_bEnabled = true;
  m_bVisible = true;
}