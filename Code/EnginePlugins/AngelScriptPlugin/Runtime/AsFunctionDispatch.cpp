#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsFunctionDispatch.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
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

    bool bPush = false;
    if (pContext->GetState() == asEContextState::asEXECUTION_ACTIVE)
    {
      bPush = true;
      AS_CHECK(pContext->PushState());
    }

    ezAngelScriptUtils::SetThreadLocalWorld(pScriptInstance->GetWorld());

    ezTime tDiff;

    if (pContext->Prepare(m_pAsFunction) >= 0)
    {
      EZ_ASSERT_DEBUG(pScriptInstance->GetObject(), "Invalid script object");
      pContext->SetObject(pScriptInstance->GetObject());

      if (m_pAsFunction->GetParamCount() > 0)
      {
        tDiff = arguments[0].Get<ezTime>();
        AS_CHECK(pContext->SetArgObject(0, &tDiff));
      }

      AS_CHECK(pContext->Execute());
    }

    if (bPush)
    {
      AS_CHECK(pContext->PopState());
    }
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

  bool bPush = false;
  if (pContext->GetState() == asEContextState::asEXECUTION_ACTIVE)
  {
    bPush = true;
    AS_CHECK(pContext->PushState());
  }

  ezAngelScriptUtils::SetThreadLocalWorld(pScriptInstance->GetWorld());

  if (pContext->Prepare(pThis->m_pAsFunction) >= 0)
  {
    EZ_ASSERT_DEBUG(pScriptInstance->GetObject(), "Invalid script object");
    AS_CHECK(pContext->SetObject(pScriptInstance->GetObject()));
    AS_CHECK(pContext->SetArgObject(0, &ref_msg));
    AS_CHECK(pContext->Execute());
  }

  if (bPush)
  {
    AS_CHECK(pContext->PopState());
  }
}

//////////////////////////////////////////////////////////////////////////

ezAngelScriptCustomAsMessageHandler::ezAngelScriptCustomAsMessageHandler(const ezScriptMessageDesc& desc)
  : ezScriptMessageHandler(desc)
{
  m_DispatchFunc = &Dispatch;
}

ezAngelScriptCustomAsMessageHandler::~ezAngelScriptCustomAsMessageHandler()
{
  for (auto& r : m_Receivers)
  {
    r.m_pAsFunction->Release();
    r.m_pAsFunction = nullptr;
  }
}


void ezAngelScriptCustomAsMessageHandler::AddReceiver(asIScriptFunction* pFunction, const char* szArgType)
{
  auto& r = m_Receivers.ExpandAndGetRef();
  r.m_pAsFunction = pFunction;
  r.m_pAsFunction->AddRef();
  r.m_sArgType.Assign(szArgType);
}

void ezAngelScriptCustomAsMessageHandler::Dispatch(ezAbstractMessageHandler* pSelf, void* pInstance, ezMessage& ref_msg)
{
  auto pThis = static_cast<ezAngelScriptCustomAsMessageHandler*>(pSelf);
  ezMsgDeliverAngelScriptMsg& asMsg = static_cast<ezMsgDeliverAngelScriptMsg&>(ref_msg);

  auto pMsgObj = reinterpret_cast<asIScriptObject*>(asMsg.m_pAsMsg);
  const char* szObjType = pMsgObj->GetObjectType()->GetName();
  const ezTempHashedString sObjType(szObjType);

  for (const auto& r : pThis->m_Receivers)
  {
    if (r.m_sArgType != sObjType)
      continue;

    auto pScriptComp = static_cast<ezScriptComponent*>(pInstance);
    auto pScriptInstance = static_cast<ezAngelScriptInstance*>(pScriptComp->GetScriptInstance());
    EZ_ASSERT_DEBUG(pScriptInstance->GetObject(), "Invalid script object");

    auto pContext = pScriptInstance->GetContext();

    bool bPush = false;
    if (pContext->GetState() == asEContextState::asEXECUTION_ACTIVE)
    {
      bPush = true;
      AS_CHECK(pContext->PushState());
    }

    ezAngelScriptUtils::SetThreadLocalWorld(pScriptInstance->GetWorld());

    if (pContext->Prepare(r.m_pAsFunction) >= 0)
    {

      AS_CHECK(pContext->SetObject(pScriptInstance->GetObject()));
      AS_CHECK(pContext->SetArgObject(0, pMsgObj));
      AS_CHECK(pContext->Execute());
    }

    if (bPush)
    {
      AS_CHECK(pContext->PopState());
    }

    break;
  }
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
