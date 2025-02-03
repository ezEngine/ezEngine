#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>
#include <Core/Scripting/ScriptRTTI.h>
#include <Foundation/Strings/String.h>

class asIScriptFunction;

class EZ_ANGELSCRIPTPLUGIN_DLL ezAngelScriptFunctionProperty : public ezScriptFunctionProperty
{
public:
  ezAngelScriptFunctionProperty(ezStringView sName, asIScriptFunction* pFunction);
  ~ezAngelScriptFunctionProperty();

  virtual ezFunctionType::Enum GetFunctionType() const override { return ezFunctionType::Member; }
  virtual const ezRTTI* GetReturnType() const override { return nullptr; }
  virtual ezBitflags<ezPropertyFlags> GetReturnFlags() const override { return ezPropertyFlags::Void; }
  virtual ezUInt32 GetArgumentCount() const override { return 0; }
  virtual const ezRTTI* GetArgumentType(ezUInt32 uiParamIndex) const override { return nullptr; }
  virtual ezBitflags<ezPropertyFlags> GetArgumentFlags(ezUInt32 uiParamIndex) const override { return ezPropertyFlags::Void; }

  virtual void Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& out_returnValue) const override;

private:
  asIScriptFunction* m_pAsFunction = nullptr;
};

class EZ_ANGELSCRIPTPLUGIN_DLL ezAngelScriptMessageHandler : public ezScriptMessageHandler
{
public:
  ezAngelScriptMessageHandler(const ezScriptMessageDesc& desc, asIScriptFunction* pFunction);
  ~ezAngelScriptMessageHandler();

  static void Dispatch(ezAbstractMessageHandler* pSelf, void* pInstance, ezMessage& ref_msg);

private:
  asIScriptFunction* m_pAsFunction = nullptr;
};
