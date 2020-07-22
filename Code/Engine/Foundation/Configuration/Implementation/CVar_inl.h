#pragma once

#include <Foundation/Configuration/CVar.h>

template <typename Type, ezCVarType::Enum CVarType>
ezTypedCVar<Type, CVarType>::ezTypedCVar(const char* szName, const Type& Value, ezBitflags<ezCVarFlags> Flags, const char* szDescription)
  : ezCVar(szName, Flags, szDescription)
{
  EZ_ASSERT_DEBUG(ezStringUtils::FindSubString(szName, " ") == nullptr, "CVar names must not contain whitespace");

  for (ezUInt32 i = 0; i < ezCVarValue::ENUM_COUNT; ++i)
    m_Values[i] = Value;
}

template <typename Type, ezCVarType::Enum CVarType>
ezTypedCVar<Type, CVarType>::operator const Type &() const
{
  return (m_Values[ezCVarValue::Current]);
}

template <typename Type, ezCVarType::Enum CVarType>
ezCVarType::Enum ezTypedCVar<Type, CVarType>::GetType() const
{
  return CVarType;
}

template <typename Type, ezCVarType::Enum CVarType>
void ezTypedCVar<Type, CVarType>::SetToRestartValue()
{
  if (m_Values[ezCVarValue::Current] == m_Values[ezCVarValue::Restart])
    return;

  // this will NOT trigger a 'restart value changed' event
  m_Values[ezCVarValue::Current] = m_Values[ezCVarValue::Restart];

  ezCVarEvent e(this);
  e.m_EventType = ezCVarEvent::ValueChanged;
  m_CVarEvents.Broadcast(e);

  // broadcast the same to the 'all cvars' event handlers
  s_AllCVarEvents.Broadcast(e);
}

template <typename Type, ezCVarType::Enum CVarType>
const Type& ezTypedCVar<Type, CVarType>::GetValue(ezCVarValue::Enum val) const
{
  return (m_Values[val]);
}

template <typename Type, ezCVarType::Enum CVarType>
void ezTypedCVar<Type, CVarType>::operator=(const Type& value)
{
  ezCVarEvent e(this);

  if (GetFlags().IsAnySet(ezCVarFlags::RequiresRestart))
  {
    if (value == m_Values[ezCVarValue::Restart]) // no change
      return;

    e.m_EventType = ezCVarEvent::RestartValueChanged;
  }
  else
  {
    if (m_Values[ezCVarValue::Current] == value) // no change
      return;

    m_Values[ezCVarValue::Current] = value;
    e.m_EventType = ezCVarEvent::ValueChanged;
  }

  m_Values[ezCVarValue::Restart] = value;

  m_CVarEvents.Broadcast(e);

  // broadcast the same to the 'all cvars' event handlers
  s_AllCVarEvents.Broadcast(e);
}
