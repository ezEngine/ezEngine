#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class ezPhantomRTTI : public ezRTTI
{
  friend class ezPhantomRttiManager;

public:
  ~ezPhantomRTTI();

private:
  ezPhantomRTTI(ezStringView sName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt8 uiVariantType,
    ezBitflags<ezTypeFlags> flags, ezStringView sPluginName);

  void SetProperties(ezDynamicArray<ezReflectedPropertyDescriptor>& properties);
  void SetFunctions(ezDynamicArray<ezReflectedFunctionDescriptor>& functions);
  void SetAttributes(ezDynamicArray<const ezPropertyAttribute*>& attributes);
  bool IsEqualToDescriptor(const ezReflectedTypeDescriptor& desc);

  void UpdateType(ezReflectedTypeDescriptor& desc);

private:
  ezString m_sTypeNameStorage;
  ezString m_sPluginNameStorage;
  ezDynamicArray<ezAbstractProperty*> m_PropertiesStorage;
  ezDynamicArray<ezAbstractFunctionProperty*> m_FunctionsStorage;
  ezDynamicArray<const ezPropertyAttribute*> m_AttributesStorage;
};
