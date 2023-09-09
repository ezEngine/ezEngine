#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/DynamicBitflags.h>

ezMap<ezString, ezDynamicBitflags> ezDynamicBitflags::s_DynamicBitflags;

void ezDynamicBitflags::Clear()
{
  m_ValidValues.Clear();
}

void ezDynamicBitflags::SetBitPosAndName(ezInt32 iBitPos, const char* szNewName)
{
  EZ_ASSERT_DEV(iBitPos >= 0 && iBitPos < 64, "Only up to 64 bits is supported.");
  auto it = m_ValidValues.FindOrAdd(EZ_BIT(iBitPos));
  it.Value() = szNewName;
}

void ezDynamicBitflags::RemoveValue(ezInt32 iBitPos)
{
  EZ_ASSERT_DEV(iBitPos >= 0 && iBitPos < 64, "Only up to 64 bits is supported.");
  m_ValidValues.Remove(EZ_BIT(iBitPos));
}

bool ezDynamicBitflags::IsValueValid(ezInt32 iBitPos) const
{
  return m_ValidValues.Find(EZ_BIT(iBitPos)).IsValid();
}

const char* ezDynamicBitflags::GetBitName(ezInt32 iBitPos) const
{
  auto it = m_ValidValues.Find(EZ_BIT(iBitPos));

  if (!it.IsValid())
    return nullptr;

  return it.Value();
}

ezDynamicBitflags& ezDynamicBitflags::GetDynamicBitflags(const char* szName)
{
  return s_DynamicBitflags[szName];
}
