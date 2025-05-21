#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GameEngine/Physics/WeightCategory.h>

ezWeightCategoryConfig::ezWeightCategoryConfig() = default;
ezWeightCategoryConfig::~ezWeightCategoryConfig() = default;

ezUInt8 ezWeightCategoryConfig::FindByName(ezTempHashedString sName) const
{
  m_Categories.Sort();

  for (ezUInt32 idx = 0; idx < m_Categories.GetCount(); ++idx)
  {
    const auto& item = m_Categories.GetValue(idx);
    if (item.m_sName == sName)
    {
      return m_Categories.GetKey(idx);
    }
  }

  return InvalidKey;
}

ezUInt8 ezWeightCategoryConfig::GetFreeKey() const
{
  m_Categories.Sort();
  for (ezUInt8 idx = FirstValidKey; idx < 250; ++idx)
  {
    if (!m_Categories.Contains(idx))
      return idx;
  }

  return InvalidKey;
}

ezResult ezWeightCategoryConfig::Save(ezStringView sFile /*= s_sConfigFile*/) const
{
  ezFileWriter file;
  if (file.Open(sFile).Failed())
    return EZ_FAILURE;

  Save(file);

  return EZ_SUCCESS;
}

ezResult ezWeightCategoryConfig::Load(ezStringView sFile /*= s_sConfigFile*/)
{
  ezFileReader file;
  if (file.Open(sFile).Failed())
    return EZ_FAILURE;

  Load(file);
  return EZ_SUCCESS;
}

void ezWeightCategoryConfig::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  m_Categories.Sort();
  const ezUInt16 uiNumCats = m_Categories.GetCount();

  inout_stream << uiNumCats;

  for (ezUInt32 i = 0; i < uiNumCats; ++i)
  {
    const auto& cat = m_Categories.GetPair(i);

    inout_stream << cat.key;
    inout_stream << cat.value.m_sName;
    inout_stream << cat.value.m_fMass;
    inout_stream << cat.value.m_sDescription;
  }
}

void ezWeightCategoryConfig::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= 1, "Invalid version '{0}' for ezWeightCategoryConfig file", uiVersion);

  ezUInt16 uiNumCats = 0;
  inout_stream >> uiNumCats;

  m_Categories.Clear();
  m_Categories.Reserve(uiNumCats);

  for (ezUInt32 i = 0; i < uiNumCats; ++i)
  {
    ezUInt8 idx = 0;
    inout_stream >> idx;

    auto& item = m_Categories[idx];
    inout_stream >> item.m_sName;

    inout_stream >> item.m_fMass;
    inout_stream >> item.m_sDescription;
  }

  m_Categories.Sort();
}

float ezWeightCategoryConfig::GetMassForWeightCategory(ezUInt8 uiWeightCategory, float fDefaultMass, float fCustomMass, float fWeightScale, float fMinMass, float fMaxMass) const
{
  if (uiWeightCategory == ezWeightCategoryConfig::DefaultValueKey)
    return fDefaultMass;

  if (uiWeightCategory == ezWeightCategoryConfig::CustomMassKey)
    return fCustomMass;

  if (uiWeightCategory == ezWeightCategoryConfig::CustomDensityKey)
    return 0.0f; // use zero mass, to calculate it from density instead

  auto& cat = m_Categories;
  ezUInt32 idx = cat.Find(uiWeightCategory);

  if (idx != ezInvalidIndex)
  {
    return ezMath::Clamp(cat.GetValue(idx).m_fMass * fWeightScale, fMinMass, fMaxMass);
  }

  return fDefaultMass;
}
