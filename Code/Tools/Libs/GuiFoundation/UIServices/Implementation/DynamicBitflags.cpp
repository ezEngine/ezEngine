#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/DynamicBitflags.h>

ezMap<ezString, ezDynamicBitflags> ezDynamicBitflags::s_DynamicBitflags;

void ezDynamicBitflags::Clear()
{
  m_ValidValues.Clear();
}

void ezDynamicBitflags::SetValueAndName(ezUInt32 uiBitPos, ezStringView sName)
{
  EZ_ASSERT_DEV(uiBitPos < 64, "Only up to 64 bits is supported.");
  auto it = m_ValidValues.FindOrAdd(EZ_BIT(uiBitPos));
  it.Value() = sName;
}

void ezDynamicBitflags::RemoveValue(ezUInt32 uiBitPos)
{
  EZ_ASSERT_DEV(uiBitPos < 64, "Only up to 64 bits is supported.");
  m_ValidValues.Remove(EZ_BIT(uiBitPos));
}

bool ezDynamicBitflags::IsValueValid(ezUInt32 uiBitPos) const
{
  return m_ValidValues.Find(EZ_BIT(uiBitPos)).IsValid();
}

bool ezDynamicBitflags::TryGetValueName(ezUInt32 uiBitPos, ezStringView& out_sName) const
{
  auto it = m_ValidValues.Find(EZ_BIT(uiBitPos));
  if (it.IsValid())
  {
    out_sName = it.Value();
    return true;
  }
  return false;
}

ezDynamicBitflags& ezDynamicBitflags::GetDynamicBitflags(ezStringView sName)
{
  return s_DynamicBitflags[sName];
}
