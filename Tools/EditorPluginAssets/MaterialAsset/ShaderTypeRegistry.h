#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Time/Timestamp.h>

class ezShaderTypeRegistry
{
  EZ_DECLARE_SINGLETON(ezShaderTypeRegistry);

public:
  ezShaderTypeRegistry();

  const ezRTTI* GetShaderType(const char* szShaderPath);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, ShaderTypeRegistry);

  struct ShaderData
  {
    ShaderData() : m_pType(nullptr) {}

    ezString m_sShaderPath;
    ezString m_sAbsShaderPath;
    ezTimestamp m_fileModifiedTime;
    const ezRTTI* m_pType;
  };
  void UpdateShaderType(ShaderData& data);

  ezMap<ezString, ShaderData> m_ShaderTypes;

};
