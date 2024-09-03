#include <Core/CorePCH.h>

#include <Core/GameState/StateMap.h>

ezStateMap::ezStateMap() = default;
ezStateMap::~ezStateMap() = default;


void ezStateMap::Clear()
{
  m_Bools.Clear();
  m_Integers.Clear();
  m_Doubles.Clear();
  m_Vec3s.Clear();
  m_Colors.Clear();
  m_Strings.Clear();
}

void ezStateMap::StoreBool(const ezTempHashedString& sName, bool value)
{
  m_Bools[sName] = value;
}

void ezStateMap::StoreInteger(const ezTempHashedString& sName, ezInt64 value)
{
  m_Integers[sName] = value;
}

void ezStateMap::StoreDouble(const ezTempHashedString& sName, double value)
{
  m_Doubles[sName] = value;
}

void ezStateMap::StoreVec3(const ezTempHashedString& sName, const ezVec3& value)
{
  m_Vec3s[sName] = value;
}

void ezStateMap::StoreColor(const ezTempHashedString& sName, const ezColor& value)
{
  m_Colors[sName] = value;
}

void ezStateMap::StoreString(const ezTempHashedString& sName, const ezString& value)
{
  m_Strings[sName] = value;
}

void ezStateMap::RetrieveBool(const ezTempHashedString& sName, bool& out_bValue, bool bDefaultValue /*= false*/)
{
  if (!m_Bools.TryGetValue(sName, out_bValue))
  {
    out_bValue = bDefaultValue;
  }
}

void ezStateMap::RetrieveInteger(const ezTempHashedString& sName, ezInt64& out_iValue, ezInt64 iDefaultValue /*= 0*/)
{
  if (!m_Integers.TryGetValue(sName, out_iValue))
  {
    out_iValue = iDefaultValue;
  }
}

void ezStateMap::RetrieveDouble(const ezTempHashedString& sName, double& out_fValue, double fDefaultValue /*= 0*/)
{
  if (!m_Doubles.TryGetValue(sName, out_fValue))
  {
    out_fValue = fDefaultValue;
  }
}

void ezStateMap::RetrieveVec3(const ezTempHashedString& sName, ezVec3& out_vValue, ezVec3 vDefaultValue /*= ezVec3(0)*/)
{
  if (!m_Vec3s.TryGetValue(sName, out_vValue))
  {
    out_vValue = vDefaultValue;
  }
}

void ezStateMap::RetrieveColor(const ezTempHashedString& sName, ezColor& out_value, ezColor defaultValue /*= ezColor::White*/)
{
  if (!m_Colors.TryGetValue(sName, out_value))
  {
    out_value = defaultValue;
  }
}

void ezStateMap::RetrieveString(const ezTempHashedString& sName, ezString& out_sValue, ezStringView sDefaultValue /*= {} */)
{
  if (!m_Strings.TryGetValue(sName, out_sValue))
  {
    out_sValue = sDefaultValue;
  }
}
