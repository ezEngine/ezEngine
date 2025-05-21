#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GameEngine/Physics/ImpulseType.h>

ezImpulseTypeConfig::ezImpulseTypeConfig() = default;
ezImpulseTypeConfig::~ezImpulseTypeConfig() = default;

ezUInt8 ezImpulseTypeConfig::FindByName(ezTempHashedString sName) const
{
  m_Types.Sort();

  for (ezUInt32 idx = 0; idx < m_Types.GetCount(); ++idx)
  {
    const auto& item = m_Types.GetValue(idx);
    if (item.m_sName == sName)
    {
      return m_Types.GetKey(idx);
    }
  }

  return InvalidKey;
}

ezUInt8 ezImpulseTypeConfig::GetFreeKey() const
{
  m_Types.Sort();
  for (ezUInt8 idx = FirstValidKey; idx < 250; ++idx)
  {
    if (!m_Types.Contains(idx))
      return idx;
  }

  return InvalidKey;
}

ezResult ezImpulseTypeConfig::Save(ezStringView sFile /*= s_sConfigFile*/) const
{
  ezFileWriter file;
  if (file.Open(sFile).Failed())
    return EZ_FAILURE;

  Save(file);

  return EZ_SUCCESS;
}

ezResult ezImpulseTypeConfig::Load(ezStringView sFile /*= s_sConfigFile*/)
{
  ezFileReader file;
  if (file.Open(sFile).Failed())
    return EZ_FAILURE;

  Load(file);
  return EZ_SUCCESS;
}

void ezImpulseTypeConfig::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  m_Types.Sort();
  const ezUInt16 uiNumCats = m_Types.GetCount();

  inout_stream << uiNumCats;

  for (ezUInt32 i = 0; i < uiNumCats; ++i)
  {
    const auto& cat = m_Types.GetPair(i);

    inout_stream << cat.key;
    inout_stream << cat.value.m_sName;
    inout_stream << cat.value.m_fDefaultValue;
    inout_stream << cat.value.m_sDescription;

    cat.value.m_WeightOverrides.Sort();
    inout_stream << cat.value.m_WeightOverrides.GetCount();
    for (const auto& f : cat.value.m_WeightOverrides)
    {
      inout_stream << f.key;
      inout_stream << f.value;
    }
  }
}

void ezImpulseTypeConfig::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= 1, "Invalid version '{0}' for ezImpulseTypeConfig file", uiVersion);

  ezUInt16 uiNumCats = 0;
  inout_stream >> uiNumCats;

  m_Types.Clear();
  m_Types.Reserve(uiNumCats);

  for (ezUInt32 i = 0; i < uiNumCats; ++i)
  {
    ezUInt8 idx = 0;
    inout_stream >> idx;

    auto& item = m_Types[idx];
    inout_stream >> item.m_sName;

    inout_stream >> item.m_fDefaultValue;
    inout_stream >> item.m_sDescription;

    ezUInt32 uiNum = 0;
    inout_stream >> uiNum;

    for (ezUInt32 i = 0; i < uiNum; ++i)
    {
      ezUInt8 uiKey;
      float fForce;

      inout_stream >> uiKey;
      inout_stream >> fForce;

      item.m_WeightOverrides.Insert(uiKey, fForce);
    }

    item.m_WeightOverrides.Sort();
  }

  m_Types.Sort();
}

float ezImpulseTypeConfig::GetImpulseForWeight(ezUInt8 uiImpulseType, ezUInt8 uiWeightCategory) const
{
  if (uiImpulseType == ezImpulseTypeConfig::NoValueKey)
    return 0.0f;

  if (uiImpulseType == ezImpulseTypeConfig::CustomValueKey)
    return 1.0f;

  // if the impulse is given as a type, look up the WeightCategory-specific impulse
  const ezUInt32 impIdx = m_Types.Find(uiImpulseType);
  if (impIdx == ezInvalidIndex)
    return 1.0f;

  const auto& impulse = m_Types.GetValue(impIdx);

  const ezUInt32 weightIdx = impulse.m_WeightOverrides.Find(uiWeightCategory);
  if (weightIdx == ezInvalidIndex)
    return impulse.m_fDefaultValue;

  // override the impulse for this weight category
  return impulse.m_WeightOverrides.GetValue(weightIdx);
}
