#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AngelScriptFunctionProperty.h>
#include <AngelScriptPlugin/Runtime/AngelScriptInstance.h>
#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>

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

  if (pContext->Prepare(pThis->m_pAsFunction) >= 0)
  {
    EZ_ASSERT_DEBUG(pScriptInstance->GetObject(), "Invalid script object");
    pContext->SetObject(pScriptInstance->GetObject());
    pContext->SetArgObject(0, &ref_msg);
    pContext->Execute();
  }
}
