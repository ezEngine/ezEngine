#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/Declarations.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

/// \brief A simple registry that stores name/value pairs of types that are common to store game state
///
class EZ_GAMEENGINE_DLL ezStateMap
{
public:
  ezStateMap();
  ~ezStateMap();

  ///void Load(ezStreamReader& stream);
  ///void Save(ezStreamWriter& stream) const;
  /// Lock / Unlock

  void Clear();

  void StoreBool(const ezTempHashedString& name, bool value);
  void StoreInteger(const ezTempHashedString& name, ezInt64 value);
  void StoreDouble(const ezTempHashedString& name, double value);
  void StoreVec3(const ezTempHashedString& name, const ezVec3& value);
  void StoreColor(const ezTempHashedString& name, const ezColor& value);
  void StoreString(const ezTempHashedString& name, const ezString& value);

  void RetrieveBool(const ezTempHashedString& name, bool& out_Value, bool defaultValue = false);
  void RetrieveInteger(const ezTempHashedString& name, ezInt64& out_Value, ezInt64 defaultValue = 0);
  void RetrieveDouble(const ezTempHashedString& name, double& out_Value, double defaultValue = 0);
  void RetrieveVec3(const ezTempHashedString& name, ezVec3& out_Value, ezVec3 defaultValue = ezVec3(0));
  void RetrieveColor(const ezTempHashedString& name, ezColor& out_Value, ezColor defaultValue = ezColor::White);
  void RetrieveString(const ezTempHashedString& name, ezString& out_Value, const char* defaultValue = nullptr);

private:
  ezHashTable<ezTempHashedString, bool> m_Bools;
  ezHashTable<ezTempHashedString, ezInt64> m_Integers;
  ezHashTable<ezTempHashedString, double> m_Doubles;
  ezHashTable<ezTempHashedString, ezVec3> m_Vec3s;
  ezHashTable<ezTempHashedString, ezColor> m_Colors;
  ezHashTable<ezTempHashedString, ezString> m_Strings;
};

