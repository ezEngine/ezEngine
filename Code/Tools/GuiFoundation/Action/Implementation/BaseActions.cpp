#include <PCH.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <Foundation/Strings/TranslationLookup.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezNamedAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCategoryAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMenuAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDynamicMenuAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDynamicActionAndMenuAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEnumerationMenuAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezButtonAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSliderAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;


ezDynamicActionAndMenuAction::ezDynamicActionAndMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath) : ezDynamicMenuAction(context, szName, szIconPath)
{
  m_bEnabled = true;
  m_bVisible = true;
}

ezEnumerationMenuAction::ezEnumerationMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath) : ezDynamicMenuAction(context, szName, szIconPath)
{
  m_pEnumerationType = nullptr;
}

void ezEnumerationMenuAction::InitEnumerationType(const ezRTTI* pEnumerationType)
{
  m_pEnumerationType = pEnumerationType;

}

void ezEnumerationMenuAction::GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();
  out_Entries.Reserve(m_pEnumerationType->GetProperties().GetCount() - 1);
  ezInt64 iCurrentValue = ezReflectionUtils::MakeEnumerationValid(m_pEnumerationType, GetValue());

  for (auto pProp : m_pEnumerationType->GetProperties().GetSubArray(1))
  {
    if (pProp->GetCategory() == ezPropertyCategory::Constant)
    {
      ezInt64 iValue = static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<ezInt64>();
      ezDynamicMenuAction::Item item;

      item.m_sDisplay = ezTranslate(pProp->GetPropertyName());

      item.m_UserValue = iValue;
      if (m_pEnumerationType->IsDerivedFrom<ezEnumBase>())
      {
        item.m_CheckState = (iCurrentValue == iValue) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
      }
      else if (m_pEnumerationType->IsDerivedFrom<ezBitflagsBase>())
      {
        item.m_CheckState = ((iCurrentValue & iValue) != 0) ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;
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


ezSliderAction::ezSliderAction(const ezActionContext& context, const char* szName)
  : ezNamedAction (context, szName, nullptr)
{
  m_bEnabled = true;
  m_bVisible = true;
  m_iMinValue = 0;
  m_iMaxValue = 100;
  m_iCurValue = 50;
}

void ezSliderAction::SetRange(ezInt32 iMin, ezInt32 iMax, bool bTriggerUpdate /*= true*/)
{
  EZ_ASSERT_DEBUG(iMin < iMax, "Invalid range");

  m_iMinValue = iMin;
  m_iMaxValue = iMax;

  if (bTriggerUpdate)
    TriggerUpdate();
}

void ezSliderAction::SetValue(ezInt32 val, bool bTriggerUpdate /*= true*/)
{
  m_iCurValue = ezMath::Clamp(val, m_iMinValue, m_iMaxValue);
  if (bTriggerUpdate)
    TriggerUpdate();
}
