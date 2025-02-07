#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsFunctionDispatch.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgDeliverAngelScriptMsg);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgDeliverAngelScriptMsg, 1, ezRTTIDefaultAllocator<ezMsgDeliverAngelScriptMsg>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAngelScriptFunctionProperty::ezAngelScriptFunctionProperty(ezStringView sName, asIScriptFunction* pFunction)
  : ezScriptFunctionProperty(sName)
{
  m_pAsFunction = pFunction;
  m_pAsFunction->AddRef();
}

ezAngelScriptFunctionProperty::~ezAngelScriptFunctionProperty()
{
  if (m_pAsFunction)
  {
    m_pAsFunction->Release();
    m_pAsFunction = nullptr;
  }
}

void ezAngelScriptFunctionProperty::Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& out_returnValue) const
{
  if (m_pAsFunction)
  {
    auto pScriptInstance = static_cast<ezAngelScriptInstance*>(pInstance);
    auto pContext = pScriptInstance->GetContext();
    pContext->PushState();

    if (pContext->Prepare(m_pAsFunction) >= 0)
    {
      EZ_ASSERT_DEBUG(pScriptInstance->GetObject(), "Invalid script object");
      pContext->SetObject(pScriptInstance->GetObject());
      pContext->Execute();
    }

    pContext->PopState();
  }
}

//////////////////////////////////////////////////////////////////////////

ezAngelScriptMessageHandler::ezAngelScriptMessageHandler(const ezScriptMessageDesc& desc, asIScriptFunction* pFunction)
  : ezScriptMessageHandler(desc)
{
  m_DispatchFunc = &Dispatch;

  m_pAsFunction = pFunction;
  m_pAsFunction->AddRef();
}

ezAngelScriptMessageHandler::~ezAngelScriptMessageHandler()
{
  if (m_pAsFunction)
  {
    m_pAsFunction->Release();
    m_pAsFunction = nullptr;
  }
}

void ezAngelScriptMessageHandler::Dispatch(ezAbstractMessageHandler* pSelf, void* pInstance, ezMessage& ref_msg)
{
  auto pScriptComp = static_cast<ezScriptComponent*>(pInstance);

  auto pThis = static_cast<ezAngelScriptMessageHandler*>(pSelf);
  auto pScriptInstance = static_cast<ezAngelScriptInstance*>(pScriptComp->GetScriptInstance());
  auto pContext = pScriptInstance->GetContext();
  pContext->PushState();

  if (pContext->Prepare(pThis->m_pAsFunction) >= 0)
  {
    EZ_ASSERT_DEBUG(pScriptInstance->GetObject(), "Invalid script object");
    pContext->SetObject(pScriptInstance->GetObject());
    pContext->SetArgObject(0, &ref_msg);
    pContext->Execute();
  }

  pContext->PopState();
}

//////////////////////////////////////////////////////////////////////////

ezAngelScriptCustomAsMessageHandler::ezAngelScriptCustomAsMessageHandler(const ezScriptMessageDesc& desc, asIScriptFunction* pFunction)
  : ezScriptMessageHandler(desc)
{
  m_DispatchFunc = &Dispatch;

  m_pAsFunction = pFunction;
  m_pAsFunction->AddRef();
}

ezAngelScriptCustomAsMessageHandler::~ezAngelScriptCustomAsMessageHandler()
{
  if (m_pAsFunction)
  {
    m_pAsFunction->Release();
    m_pAsFunction = nullptr;
  }
}

void ezAngelScriptCustomAsMessageHandler::Dispatch(ezAbstractMessageHandler* pSelf, void* pInstance, ezMessage& ref_msg)
{
  auto pScriptComp = static_cast<ezScriptComponent*>(pInstance);

  auto pThis = static_cast<ezAngelScriptCustomAsMessageHandler*>(pSelf);
  auto pScriptInstance = static_cast<ezAngelScriptInstance*>(pScriptComp->GetScriptInstance());
  auto pContext = pScriptInstance->GetContext();
  pContext->PushState();

  if (pContext->Prepare(pThis->m_pAsFunction) >= 0)
  {
    ezMsgDeliverAngelScriptMsg& asMsg = static_cast<ezMsgDeliverAngelScriptMsg&>(ref_msg);

    EZ_ASSERT_DEBUG(pScriptInstance->GetObject(), "Invalid script object");
    pContext->SetObject(pScriptInstance->GetObject());
    pContext->SetArgObject(0, asMsg.m_pAsMsg);
    pContext->Execute();
  }

  pContext->PopState();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezMsgDeliverAngelScriptMsg::~ezMsgDeliverAngelScriptMsg()
{
  if (m_bRelease)
  {
    auto pObj = reinterpret_cast<asIScriptObject*>(m_pAsMsg);
    pObj->Release();
  }
}

ezMsgDeliverAngelScriptMsg::ezMsgDeliverAngelScriptMsg(const ezMsgDeliverAngelScriptMsg& rhs)
{
  *this = rhs;
}

ezMsgDeliverAngelScriptMsg::ezMsgDeliverAngelScriptMsg(ezMsgDeliverAngelScriptMsg&& rhs)
{
  *this = std::move(rhs);
}

void ezMsgDeliverAngelScriptMsg::operator=(const ezMsgDeliverAngelScriptMsg& rhs)
{
  ezMemoryUtils::RawByteCopy(this, &rhs, sizeof(ezMsgDeliverAngelScriptMsg));

  if (m_bRelease)
  {
    auto pObj = reinterpret_cast<asIScriptObject*>(m_pAsMsg);
    pObj->AddRef();
  }
}

void ezMsgDeliverAngelScriptMsg::operator=(ezMsgDeliverAngelScriptMsg&& rhs)
{
  m_bRelease = rhs.m_bRelease;
  m_pAsMsg = rhs.m_pAsMsg;
  rhs.m_bRelease = false;
  rhs.m_pAsMsg = nullptr;
}
