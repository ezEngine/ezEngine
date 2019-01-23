#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Timestamp.h>

class ezScriptMemberProperty : public ezAbstractMemberProperty
{
public:
  ezScriptMemberProperty(const char* szName, const ezRTTI* pType);
  ~ezScriptMemberProperty();

  virtual const ezRTTI* GetSpecificType() const override;
  virtual void* GetPropertyPointer(const void* pInstance) const override { return nullptr; }
  virtual void GetValuePtr(const void* pInstance, void* pObject) const override;
  virtual void SetValuePtr(void* pInstance, void* pObject) override;

private:
  ezString m_sPropertyNameStorage;
  const ezRTTI* m_pPropertyType;
};

class ezScriptRTTI : public ezRTTI
{
  friend class ezScriptRegistry;
public:
  ~ezScriptRTTI();

private:
  ezScriptRTTI(const char* szName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt32 uiVariantType, ezBitflags<ezTypeFlags> flags, const char* szPluginName);

  void SetProperties(ezArrayPtr<ezAbstractProperty*> properties);
  void SetAttributes(ezArrayPtr<ezPropertyAttribute*> attributes);

private:
  // ezRTTI only has ptr for these so wee need some kind of storage to hold them:
  ezString m_sTypeNameStorage;
  ezString m_sPluginNameStorage;
  ezDynamicArray<ezAbstractProperty*> m_PropertiesStorage;
  ezDynamicArray<ezPropertyAttribute*> m_AttributesStorage;
};

class ezScriptRegistry
{
  EZ_DECLARE_SINGLETON(ezScriptRegistry);

public:
  ezScriptRegistry();

  void UpdateScriptTypes(const char* szScriptPath);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ScriptRegistry);

  struct ScriptData
  {
    ScriptData() {}

    ezString m_sScriptPath;
    ezTimestamp m_fileModifiedTime;
    ezHybridArray<const ezRTTI*, 1> m_pTypes;
  };
  void UpdateScriptTypes(ScriptData& data);

  ezMap<ezString, ScriptData> m_ShaderTypes;

};
