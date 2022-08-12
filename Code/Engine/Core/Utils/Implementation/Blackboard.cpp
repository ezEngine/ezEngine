#include <Core/CorePCH.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezBlackboardEntryFlags, 1)
  EZ_BITFLAGS_CONSTANTS(ezBlackboardEntryFlags::Save, ezBlackboardEntryFlags::OnChangeEvent,
    ezBlackboardEntryFlags::UserFlag0, ezBlackboardEntryFlags::UserFlag1, ezBlackboardEntryFlags::UserFlag2, ezBlackboardEntryFlags::UserFlag3, ezBlackboardEntryFlags::UserFlag4, ezBlackboardEntryFlags::UserFlag5, ezBlackboardEntryFlags::UserFlag6, ezBlackboardEntryFlags::UserFlag7)
EZ_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezBlackboard::ezBlackboard() = default;
ezBlackboard::~ezBlackboard() = default;

void ezBlackboard::SetName(const char* szName)
{
  m_sName.Assign(szName);
}

void ezBlackboard::RegisterEntry(const ezHashedString& name, const ezVariant& initialValue, ezBitflags<ezBlackboardEntryFlags> flags /*= ezBlackboardEntryFlags::None*/)
{
  EZ_ASSERT_ALWAYS(!flags.IsSet(ezBlackboardEntryFlags::Invalid), "The invalid flag is reserved for internal use.");

  bool bExisted = false;
  Entry& entry = m_Entries.FindOrAdd(name, &bExisted);

  if (!bExisted || entry.m_Flags != flags)
  {
    ++m_uiBlackboardChangeCounter;
    entry.m_Flags |= flags;
  }

  if (entry.m_Value != initialValue)
  {
    // broadcasts the change event, in case we overwrite an existing entry
    SetEntryValue(name, initialValue).IgnoreResult();
  }
}

void ezBlackboard::UnregisterEntry(const ezHashedString& name)
{
  if (m_Entries.Remove(name))
  {
    ++m_uiBlackboardChangeCounter;
  }
}

void ezBlackboard::UnregisterAllEntries()
{
  if (m_Entries.IsEmpty() == false)
  {
    ++m_uiBlackboardChangeCounter;
  }

  m_Entries.Clear();
}

ezResult ezBlackboard::SetEntryValue(const ezTempHashedString& name, const ezVariant& value, bool force /*= false*/)
{
  auto itEntry = m_Entries.Find(name);

  if (!itEntry.IsValid())
  {
    return EZ_FAILURE;
  }

  Entry& entry = itEntry.Value();

  if (!force && entry.m_Value == value)
    return EZ_SUCCESS;

  ++m_uiBlackboardEntryChangeCounter;
  ++entry.m_uiChangeCounter;

  if (entry.m_Flags.IsSet(ezBlackboardEntryFlags::OnChangeEvent))
  {
    EntryEvent e;
    e.m_sName = itEntry.Key();
    e.m_OldValue = entry.m_Value;
    e.m_pEntry = &entry;

    entry.m_Value = value;

    m_EntryEvents.Broadcast(e, 1); // limited recursion is allowed
  }
  else
  {
    entry.m_Value = value;
  }

  return EZ_SUCCESS;
}

const ezBlackboard::Entry* ezBlackboard::GetEntry(const ezTempHashedString& name) const
{
  auto itEntry = m_Entries.Find(name);

  if (!itEntry.IsValid())
    return nullptr;

  return &itEntry.Value();
}

ezVariant ezBlackboard::GetEntryValue(const ezTempHashedString& name) const
{
  auto value = m_Entries.GetValue(name);
  return value != nullptr ? value->m_Value : ezVariant();
}

ezBitflags<ezBlackboardEntryFlags> ezBlackboard::GetEntryFlags(const ezTempHashedString& name) const
{
  auto itEntry = m_Entries.Find(name);

  if (!itEntry.IsValid())
  {
    return ezBlackboardEntryFlags::Invalid;
  }

  return itEntry.Value().m_Flags;
}

ezResult ezBlackboard::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  ezUInt32 uiEntries = 0;

  for (auto it : m_Entries)
  {
    if (it.Value().m_Flags.IsSet(ezBlackboardEntryFlags::Save))
    {
      ++uiEntries;
    }
  }

  stream << uiEntries;

  for (auto it : m_Entries)
  {
    const Entry& e = it.Value();

    if (e.m_Flags.IsSet(ezBlackboardEntryFlags::Save))
    {
      stream << it.Key();
      stream << e.m_Flags;
      stream << e.m_Value;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezBlackboard::Deserialize(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  ezUInt32 uiEntries = 0;
  stream >> uiEntries;

  for (ezUInt32 e = 0; e < uiEntries; ++e)
  {
    ezHashedString name;
    stream >> name;

    ezBitflags<ezBlackboardEntryFlags> flags;
    stream >> flags;

    ezVariant value;
    stream >> value;

    RegisterEntry(name, value, flags);
  }

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezComparisonOperator, 1)
  EZ_ENUM_CONSTANTS(ezComparisonOperator::Equal, ezComparisonOperator::NotEqual)
  EZ_ENUM_CONSTANTS(ezComparisonOperator::Less, ezComparisonOperator::LessEqual)
  EZ_ENUM_CONSTANTS(ezComparisonOperator::Greater, ezComparisonOperator::GreaterEqual)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
bool ezComparisonOperator::Compare(ezComparisonOperator::Enum cmp, double f1, double f2)
{
  switch (cmp)
  {
    case ezComparisonOperator::Equal:
      return f1 == f2;
    case ezComparisonOperator::NotEqual:
      return f1 != f2;
    case ezComparisonOperator::Less:
      return f1 < f2;
    case ezComparisonOperator::LessEqual:
      return f1 <= f2;
    case ezComparisonOperator::Greater:
      return f1 > f2;
    case ezComparisonOperator::GreaterEqual:
      return f1 >= f2;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezBlackboardCondition, ezNoBase, 1, ezRTTIDefaultAllocator<ezBlackboardCondition>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("EntryName", GetEntryName, SetEntryName),
    EZ_ENUM_MEMBER_PROPERTY("Operator", ezComparisonOperator, m_Operator),
    EZ_MEMBER_PROPERTY("ComparisonValue", m_fComparisonValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

bool ezBlackboardCondition::IsConditionMet(const ezBlackboard& blackboard) const
{
  auto pEntry = blackboard.GetEntry(m_sEntryName);
  if (pEntry != nullptr && pEntry->m_Value.IsNumber())
  {
    double fEntryValue = pEntry->m_Value.ConvertTo<double>();
    return ezComparisonOperator::Compare(m_Operator, fEntryValue, m_fComparisonValue);
  }

  return false;
}


EZ_STATICLINK_FILE(Core, Core_Utils_Implementation_Blackboard);
