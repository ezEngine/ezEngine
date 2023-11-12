#pragma once

#include <VisualScriptPlugin/Runtime/VisualScript.h>

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptFunctionProperty : public ezScriptFunctionProperty
{
public:
  ezVisualScriptFunctionProperty(ezStringView sName, const ezSharedPtr<const ezVisualScriptGraphDescription>& pDesc);
  ~ezVisualScriptFunctionProperty();

  virtual ezFunctionType::Enum GetFunctionType() const override { return ezFunctionType::Member; }
  virtual const ezRTTI* GetReturnType() const override { return nullptr; }
  virtual ezBitflags<ezPropertyFlags> GetReturnFlags() const override { return ezPropertyFlags::Void; }
  virtual ezUInt32 GetArgumentCount() const override { return 0; }
  virtual const ezRTTI* GetArgumentType(ezUInt32 uiParamIndex) const override { return nullptr; }
  virtual ezBitflags<ezPropertyFlags> GetArgumentFlags(ezUInt32 uiParamIndex) const override { return ezPropertyFlags::Void; }

  virtual void Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& out_returnValue) const override;

private:
  ezSharedPtr<const ezVisualScriptGraphDescription> m_pDesc;
  mutable ezVisualScriptDataStorage m_LocalDataStorage;
};

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptMessageHandler : public ezScriptMessageHandler
{
public:
  ezVisualScriptMessageHandler(const ezScriptMessageDesc& desc, const ezSharedPtr<const ezVisualScriptGraphDescription>& pDesc);
  ~ezVisualScriptMessageHandler();

  static void Dispatch(ezAbstractMessageHandler* pSelf, void* pInstance, ezMessage& ref_msg);

private:
  ezSharedPtr<const ezVisualScriptGraphDescription> m_pDesc;
  mutable ezVisualScriptDataStorage m_LocalDataStorage;
};
