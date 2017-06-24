#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class ezPhantomRTTI : public ezRTTI
{
  friend class ezPhantomRttiManager;
public:
  ~ezPhantomRTTI();

private:
  ezPhantomRTTI(const char* szName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt32 uiVariantType, ezBitflags<ezTypeFlags> flags, const char* szPluginName);

  void SetProperties(ezDynamicArray<ezReflectedPropertyDescriptor>& properties);
  void SetFunctions(ezDynamicArray<ezReflectedFunctionDescriptor>& functions);
  void SetAttributes(ezHybridArray<ezPropertyAttribute*, 2>& attributes);
  bool IsEqualToDescriptor(const ezReflectedTypeDescriptor& desc);

  void UpdateType(ezReflectedTypeDescriptor& desc);

private:
  ezString m_sTypeNameStorage;
  ezString m_sPluginNameStorage;
  ezDynamicArray<ezAbstractProperty*> m_PropertiesStorage;
  ezDynamicArray<ezAbstractFunctionProperty*> m_FunctionsStorage;
  ezDynamicArray<ezPropertyAttribute*> m_AttributesStorage;
};
