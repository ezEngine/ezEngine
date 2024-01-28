#pragma once

#include <Foundation/Configuration/CVar.h>

template <typename Type, ezCVarType::Enum CVarType>
ezTypedCVar<Type, CVarType>::ezTypedCVar(ezStringView sName, const Type& value, ezBitflags<ezCVarFlags> flags, ezStringView sDescription)
  : ezCVar(sName, flags, sDescription)
{
  EZ_ASSERT_DEBUG(sName.FindSubString(" ") == nullptr, "CVar names must not contain whitespace");

  for (ezUInt32 i = 0; i < ezCVarValue::ENUM_COUNT; ++i)
    m_Values[i] = value;
}

template <typename Type, ezCVarType::Enum CVarType>
ezTypedCVar<Type, CVarType>::operator const Type&() const
{
  return (m_Values[ezCVarValue::Current]);
}

template <typename Type, ezCVarType::Enum CVarType>
ezCVarType::Enum ezTypedCVar<Type, CVarType>::GetType() const
{
  return CVarType;
}

template <typename Type, ezCVarType::Enum CVarType>
void ezTypedCVar<Type, CVarType>::SetToDelayedSyncValue()
{
  if (m_Values[ezCVarValue::Current] == m_Values[ezCVarValue::DelayedSync])
    return;

  // this will NOT trigger a 'restart value changed' event
  m_Values[ezCVarValue::Current] = m_Values[ezCVarValue::DelayedSync];

  ezCVarEvent e(this);
  e.m_EventType = ezCVarEvent::ValueChanged;
  m_CVarEvents.Broadcast(e);

  // broadcast the same to the 'all CVars' event handlers
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

  if (GetFlags().IsAnySet(ezCVarFlags::RequiresDelayedSync))
  {
    if (value == m_Values[ezCVarValue::DelayedSync]) // no change
      return;

    e.m_EventType = ezCVarEvent::DelayedSyncValueChanged;
  }
  else
  {
    if (m_Values[ezCVarValue::Current] == value) // no change
      return;

    m_Values[ezCVarValue::Current] = value;
    e.m_EventType = ezCVarEvent::ValueChanged;
  }

  m_Values[ezCVarValue::DelayedSync] = value;

  m_CVarEvents.Broadcast(e);

  // broadcast the same to the 'all cvars' event handlers
  s_AllCVarEvents.Broadcast(e);
}
