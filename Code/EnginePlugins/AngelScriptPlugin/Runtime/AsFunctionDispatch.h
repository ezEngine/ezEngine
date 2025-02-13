#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>
#include <Core/Scripting/ScriptRTTI.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Strings/String.h>

//////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////

class EZ_ANGELSCRIPTPLUGIN_DLL ezAngelScriptMessageHandler : public ezScriptMessageHandler
{
public:
  ezAngelScriptMessageHandler(const ezScriptMessageDesc& desc, asIScriptFunction* pFunction);
  ~ezAngelScriptMessageHandler();

  static void Dispatch(ezAbstractMessageHandler* pSelf, void* pInstance, ezMessage& ref_msg);

private:
  asIScriptFunction* m_pAsFunction = nullptr;
};

//////////////////////////////////////////////////////////////////////////

struct ezMsgDeliverAngelScriptMsg : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgDeliverAngelScriptMsg, ezMessage);

  ~ezMsgDeliverAngelScriptMsg();

  ezMsgDeliverAngelScriptMsg(const ezMsgDeliverAngelScriptMsg& rhs);
  ezMsgDeliverAngelScriptMsg(ezMsgDeliverAngelScriptMsg&& rhs);
  void operator=(const ezMsgDeliverAngelScriptMsg& rhs);
  void operator=(ezMsgDeliverAngelScriptMsg&& rhs);

  bool m_bRelease = false;
  void* m_pAsMsg = nullptr;
};

class EZ_ANGELSCRIPTPLUGIN_DLL ezAngelScriptCustomAsMessageHandler : public ezScriptMessageHandler
{
public:
  ezAngelScriptCustomAsMessageHandler(const ezScriptMessageDesc& desc);
  ~ezAngelScriptCustomAsMessageHandler();

  void AddReceiver(asIScriptFunction* pFunction, const char* szArgType);

  static void Dispatch(ezAbstractMessageHandler* pSelf, void* pInstance, ezMessage& ref_msg);

private:
  struct Receiver
  {
    ezHashedString m_sArgType;
    asIScriptFunction* m_pAsFunction = nullptr;
  };

  ezHybridArray<Receiver, 2> m_Receivers;
};
