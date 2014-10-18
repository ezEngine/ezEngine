#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Types/Bitflags.h>

struct ezSettingsFlags
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None = 0,
    Registered = EZ_BIT(0), // internal
    Hidden = EZ_BIT(1),
    User = EZ_BIT(2),
  };

  struct Bits
  {
    StorageType Registered : 1;
    StorageType Hidden : 1;
    StorageType User : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezSettingsFlags);

class EZ_EDITORFRAMEWORK_DLL ezSettings
{
public:

  void RegisterValueBool(const char* szKey, bool Default, ezBitflags<ezSettingsFlags> flags = ezSettingsFlags::None);
  void RegisterValueInt(const char* szKey, ezInt32 Default, ezBitflags<ezSettingsFlags> flags = ezSettingsFlags::None);
  void RegisterValueFloat(const char* szKey, float Default, ezBitflags<ezSettingsFlags> flags = ezSettingsFlags::None);
  void RegisterValueString(const char* szKey, const char* Default, ezBitflags<ezSettingsFlags> flags = ezSettingsFlags::None);
  void RegisterValueColor(const char* szKey, const ezColor& Default, ezBitflags<ezSettingsFlags> flags = ezSettingsFlags::None);

  void SetValueBool(const char* szKey, bool value);
  void SetValueInt(const char* szKey, ezInt32 value);
  void SetValueFloat(const char* szKey, float value);
  void SetValueString(const char* szKey, const char* value);
  void SetValueColor(const char* szKey, const ezColor& value);

  bool GetValueBool(const char* szKey);
  ezInt32 GetValueInt(const char* szKey);
  float GetValueFloat(const char* szKey);
  ezString GetValueString(const char* szKey);
  ezColor GetValueColor(const char* szKey);

  void WriteToJSON(ezStreamWriterBase& stream, bool bNonUserSettings, bool bUserSettings) const;
  void ReadFromJSON(ezStreamReaderBase& stream);

private:
  struct VariableValue
  {
    ezBitflags<ezSettingsFlags> m_Flags;
    ezVariant m_Value;
  };

  ezMap<ezString, VariableValue> m_Settings;

};

