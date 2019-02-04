#include <PCH.h>

#include <GameEngine/GameState/StateMap.h>

ezStateMap::ezStateMap() {}
ezStateMap::~ezStateMap() {}


void ezStateMap::Clear()
{
  m_Bools.Clear();
  m_Integers.Clear();
  m_Doubles.Clear();
  m_Vec3s.Clear();
  m_Colors.Clear();
  m_Strings.Clear();
}

void ezStateMap::StoreBool(const ezTempHashedString& name, bool value)
{
  m_Bools[name] = value;
}

void ezStateMap::StoreInteger(const ezTempHashedString& name, ezInt64 value)
{
  m_Integers[name] = value;
}

void ezStateMap::StoreDouble(const ezTempHashedString& name, double value)
{
  m_Doubles[name] = value;
}

void ezStateMap::StoreVec3(const ezTempHashedString& name, const ezVec3& value)
{
  m_Vec3s[name] = value;
}

void ezStateMap::StoreColor(const ezTempHashedString& name, const ezColor& value)
{
  m_Colors[name] = value;
}

void ezStateMap::StoreString(const ezTempHashedString& name, const ezString& value)
{
  m_Strings[name] = value;
}

void ezStateMap::RetrieveBool(const ezTempHashedString& name, bool& out_Value, bool defaultValue /*= false*/)
{
  if (!m_Bools.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void ezStateMap::RetrieveInteger(const ezTempHashedString& name, ezInt64& out_Value, ezInt64 defaultValue /*= 0*/)
{
  if (!m_Integers.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void ezStateMap::RetrieveDouble(const ezTempHashedString& name, double& out_Value, double defaultValue /*= 0*/)
{
  if (!m_Doubles.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void ezStateMap::RetrieveVec3(const ezTempHashedString& name, ezVec3& out_Value, ezVec3 defaultValue /*= ezVec3(0)*/)
{
  if (!m_Vec3s.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void ezStateMap::RetrieveColor(const ezTempHashedString& name, ezColor& out_Value, ezColor defaultValue /*= ezColor::White*/)
{
  if (!m_Colors.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void ezStateMap::RetrieveString(const ezTempHashedString& name, ezString& out_Value, const char* defaultValue /*= nullptr*/)
{
  if (!m_Strings.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_StateMap);

