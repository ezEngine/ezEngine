#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Variant.h>

class EZ_EDITORFRAMEWORK_DLL ezSettings
{
public:

  void RegisterValueBool(const char* szKey, bool Default);
  void RegisterValueInt(const char* szKey, ezInt32 Default);
  void RegisterValueFloat(const char* szKey, float Default);
  void RegisterValueString(const char* szKey, const char* Default);
  void RegisterValueColor(const char* szKey, const ezColor& Default);

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

  void WriteToJSON(ezStreamWriterBase& stream) const;
  void ReadFromJSON(ezStreamReaderBase& stream);

private:
  struct VariableValue
  {
    bool m_bRegistered;
    ezVariant m_Value;
  };

  ezMap<ezString, VariableValue> m_Settings;

};

