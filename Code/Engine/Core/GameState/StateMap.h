#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

/// A simple registry that stores name/value pairs of types that are common to store game state.
///
/// Provides type-safe storage and retrieval of common data types used in game state management.
/// Values are stored by name and can be retrieved with optional default values.
class EZ_CORE_DLL ezStateMap
{
public:
  ezStateMap();
  ~ezStateMap();

  /// void Load(ezStreamReader& stream);
  /// void Save(ezStreamWriter& stream) const;
  /// Lock / Unlock

  void Clear();

  void StoreBool(const ezTempHashedString& sName, bool value);
  void StoreInteger(const ezTempHashedString& sName, ezInt64 value);
  void StoreDouble(const ezTempHashedString& sName, double value);
  void StoreVec3(const ezTempHashedString& sName, const ezVec3& value);
  void StoreColor(const ezTempHashedString& sName, const ezColor& value);
  void StoreString(const ezTempHashedString& sName, const ezString& value);

  void RetrieveBool(const ezTempHashedString& sName, bool& out_bValue, bool bDefaultValue = false);
  void RetrieveInteger(const ezTempHashedString& sName, ezInt64& out_iValue, ezInt64 iDefaultValue = 0);
  void RetrieveDouble(const ezTempHashedString& sName, double& out_fValue, double fDefaultValue = 0);
  void RetrieveVec3(const ezTempHashedString& sName, ezVec3& out_vValue, ezVec3 vDefaultValue = ezVec3(0));
  void RetrieveColor(const ezTempHashedString& sName, ezColor& out_value, ezColor defaultValue = ezColor::White);
  void RetrieveString(const ezTempHashedString& sName, ezString& out_sValue, ezStringView sDefaultValue = {});

private:
  ezHashTable<ezTempHashedString, bool> m_Bools;
  ezHashTable<ezTempHashedString, ezInt64> m_Integers;
  ezHashTable<ezTempHashedString, double> m_Doubles;
  ezHashTable<ezTempHashedString, ezVec3> m_Vec3s;
  ezHashTable<ezTempHashedString, ezColor> m_Colors;
  ezHashTable<ezTempHashedString, ezString> m_Strings;
};
