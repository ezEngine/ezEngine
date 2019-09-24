#include <TypeScriptPluginPCH.h>

#include <Foundation/Threading/DelegateTask.h>
#include <TypeScriptPlugin/Components/TypeScriptWrapper.h>

int TS_ezInternal_ezTsComponent_GetOwner(duk_context* pContext);
int TS_ezInternal_ezTsGameObject_SetLocalPosition(duk_context* pContext);
int TS_ezLog_Info(duk_context* pContext);

ezTaskGroupID ezTypeScriptWrapper::s_LoadTranspilerTask;
ezDuktapeWrapper ezTypeScriptWrapper::s_Transpiler("TsTranspiler");

ezTypeScriptWrapper::ezTypeScriptWrapper()
  : m_Script("TsScript")
{
}

void ezTypeScriptWrapper::Initialize(ezWorld* pWorld)
{
  m_Script.EnableModuleSupport(ezTypeScriptWrapper::DukSearchModule);

  m_Script.RegisterFunction("ezInternal_ezTsComponent_GetOwner", TS_ezInternal_ezTsComponent_GetOwner, 1);
  m_Script.RegisterFunction("ezInternal_ezTsGameObject_SetLocalPosition", TS_ezInternal_ezTsGameObject_SetLocalPosition, 4);

  // store ezWorld* in global stash
  {
    m_Script.OpenGlobalStashObject();

    ezWorld** pWorldBuffer = reinterpret_cast<ezWorld**>(duk_push_fixed_buffer(m_Script.GetContext(), sizeof(void*)));
    *pWorldBuffer = pWorld;

    duk_put_prop_index(m_Script.GetContext(), -2, 0 /* index for ezWorld* */);

    m_Script.CloseObject();
  }
}

void ezTypeScriptWrapper::SetupScript()
{
  ezStringBuilder js;
  if (ezTypeScriptWrapper::TranspileFile("TypeScript/Component.ts", js).Failed())
    return;

  SetModuleSearchPath("TypeScript");

  if (m_Script.ExecuteString(js, "TypeScript/Component.ts").Failed())
    return;

  if (m_Script.OpenObject("ez").Succeeded())
  {
    if (m_Script.OpenObject("Log").Succeeded())
    {
      m_Script.RegisterFunction("Error", TS_ezLog_Info, 1, ezLogMsgType::ErrorMsg);
      m_Script.RegisterFunction("SeriousWarning", TS_ezLog_Info, 1, ezLogMsgType::SeriousWarningMsg);
      m_Script.RegisterFunction("Warning", TS_ezLog_Info, 1, ezLogMsgType::WarningMsg);
      m_Script.RegisterFunction("Success", TS_ezLog_Info, 1, ezLogMsgType::SuccessMsg);
      m_Script.RegisterFunction("Info", TS_ezLog_Info, 1, ezLogMsgType::InfoMsg);
      m_Script.RegisterFunction("Dev", TS_ezLog_Info, 1, ezLogMsgType::DevMsg);
      m_Script.RegisterFunction("Debug", TS_ezLog_Info, 1, ezLogMsgType::DebugMsg);
      m_Script.CloseObject();
    }
    m_Script.CloseObject();
  }
}

void ezTypeScriptWrapper::SetModuleSearchPath(const char* szPath)
{
  m_sSearchPath = szPath;

  {
    m_Script.OpenGlobalStashObject();

    duk_push_string(m_Script, szPath);
    EZ_ASSERT_DEV(duk_put_prop_string(m_Script, -2, "ModuleSearchPath"), "");

    m_Script.CloseObject();
  }
}

void ezTypeScriptWrapper::StartLoadTranspiler()
{
  if (s_LoadTranspilerTask.IsValid())
    return;

  ezDelegateTask<void>* pTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "Load Transpiler", []() //
    {
      EZ_VERIFY(s_Transpiler.ExecuteFile("typescriptServices.js").Succeeded(), "");
    });

  pTask->SetOnTaskFinished([](ezTask* pTask) { EZ_DEFAULT_DELETE(pTask); });
  s_LoadTranspilerTask = ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::LongRunning);
}

void ezTypeScriptWrapper::FinishLoadTranspiler()
{
  StartLoadTranspiler();

  ezTaskSystem::WaitForGroup(s_LoadTranspilerTask);
}

ezResult ezTypeScriptWrapper::TranspileFile(const char* szFile, ezStringBuilder& result)
{
  FinishLoadTranspiler();

  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  result.ReadAll(file);

  EZ_SUCCEED_OR_RETURN(s_Transpiler.OpenObject("ts"));
  EZ_SUCCEED_OR_RETURN(s_Transpiler.BeginFunctionCall("transpile"));
  s_Transpiler.PushParameter(result);
  EZ_SUCCEED_OR_RETURN(s_Transpiler.ExecuteFunctionCall());

  result = s_Transpiler.GetStringReturnValue();

  s_Transpiler.EndFunctionCall();
  s_Transpiler.CloseObject();

  ezStringBuilder sOutFile = szFile;
  sOutFile.ChangeFileExtension("js");
  sOutFile.Prepend(":project/");

  ezFileWriter fileOut;
  fileOut.Open(sOutFile);
  fileOut.WriteBytes(result.GetData(), result.GetElementCount());

  return EZ_SUCCESS;
}

int TS_ezLog_Info(duk_context* pContext)
{
  ezDuktapeFunction wrapper(pContext);
  const ezInt16 iMagic = wrapper.GetFunctionMagicValue();

  switch (iMagic)
  {
    case ezLogMsgType::ErrorMsg:
      ezLog::Error(wrapper.GetStringParameter(0));
      break;
    case ezLogMsgType::SeriousWarningMsg:
      ezLog::SeriousWarning(wrapper.GetStringParameter(0));
      break;
    case ezLogMsgType::WarningMsg:
      ezLog::Warning(wrapper.GetStringParameter(0));
      break;
    case ezLogMsgType::SuccessMsg:
      ezLog::Success(wrapper.GetStringParameter(0));
      break;
    case ezLogMsgType::InfoMsg:
      ezLog::Info(wrapper.GetStringParameter(0));
      break;
    case ezLogMsgType::DevMsg:
      ezLog::Dev(wrapper.GetStringParameter(0));
      break;
    case ezLogMsgType::DebugMsg:
      ezLog::Debug(wrapper.GetStringParameter(0));
      break;
  }

  return wrapper.ReturnVoid();
}

int TS_ezInternal_ezTsComponent_GetOwner(duk_context* pContext)
{
  ezDuktapeFunction wrapper(pContext);

  duk_require_object(pContext, 0);
  duk_get_prop_string(pContext, 0, "ezComponentPtr");
  ezComponent* pComponent = (ezComponent*)duk_get_pointer(pContext, -1);
  duk_pop(pContext);

  //ezLog::Info("ezTsComponent::GetOwner -> {}", pComponent->GetOwner()->GetName());

  // create ezTsGameObject and store ezGameObject Ptr and Handle in it
  wrapper.OpenGlobalObject();
  EZ_VERIFY(wrapper.BeginFunctionCall("_ezTS_CreateGameObject").Succeeded(), "");
  EZ_VERIFY(wrapper.ExecuteFunctionCall().Succeeded(), "");
  duk_dup_top(pContext);
  wrapper.EndFunctionCall();

  {
    duk_push_pointer(pContext, pComponent->GetOwner()); // TODO: use handle
    duk_put_prop_string(pContext, -2, "ezGameObjectPtr");
  }

  {
    ezGameObjectHandle* pHandleBuffer = reinterpret_cast<ezGameObjectHandle*>(duk_push_fixed_buffer(pContext, sizeof(ezGameObjectHandle)));
    *pHandleBuffer = pComponent->GetOwner()->GetHandle();
    duk_put_prop_string(pContext, -2, "ezGameObjectHandle");
  }

  return wrapper.ReturnCustom();
}

int TS_ezInternal_ezTsGameObject_SetLocalPosition(duk_context* pContext)
{
  ezDuktapeFunction wrapper(pContext);
  EZ_VERIFY(wrapper.IsParameterObject(0), "");

  ezWorld* pWorld = nullptr;

  // retrieve ezWorld* in global stash
  {
    // TODO: look up ezWorld* externally instead of through the stash

    wrapper.OpenGlobalStashObject();

    duk_get_prop_index(pContext, -1, 0 /* index for ezWorld* */);
    pWorld = *reinterpret_cast<ezWorld**>(duk_get_buffer(pContext, -1, nullptr));
    duk_pop(pContext);

    wrapper.CloseObject();
  }

  ezGameObjectHandle hObject;
  ezGameObject* pGameObject = nullptr;

  {
    duk_get_prop_string(pContext, 0, "ezGameObjectHandle");
    hObject = *reinterpret_cast<ezGameObjectHandle*>(duk_get_buffer(pContext, -1, nullptr));
    duk_pop(pContext);

    EZ_VERIFY(pWorld->TryGetObject(hObject, pGameObject), "");
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  {
    duk_get_prop_string(pContext, 0, "ezGameObjectPtr");
    ezGameObject* pGo = (ezGameObject*)duk_get_pointer_default(pContext, -1, nullptr);
    duk_pop(pContext);

    EZ_VERIFY(pGo == pGameObject, "outdated pointer");
  }
#endif

  if (pGameObject)
  {
    EZ_VERIFY(pWorld == pGameObject->GetWorld(), "");

    ezVec3 pos(wrapper.GetFloatParameter(1), wrapper.GetFloatParameter(2), wrapper.GetFloatParameter(3));
    pGameObject->SetLocalPosition(pos);
  }

  return wrapper.ReturnVoid();
}

int ezTypeScriptWrapper::DukSearchModule(duk_context* pContext)
{
  ezDuktapeFunction script(pContext);

  /* 
  *   index 0: id
  *   index 1: require
  *   index 2: exports
  *   index 3: module
  */

  ezStringBuilder sRequestedFile = script.GetStringParameter(0);

  if (!sRequestedFile.HasAnyExtension())
  {
    sRequestedFile.ChangeFileExtension("ts");
  }

  // retrieve the ModuleSearchPath
  {
    script.OpenGlobalStashObject();

    EZ_ASSERT_DEV(duk_get_prop_string(script, -1, "ModuleSearchPath"), "");
    const char* szSearchPath = duk_get_string_default(script, -1, "");

    if (!ezStringUtils::IsNullOrEmpty(szSearchPath))
    {
      sRequestedFile.Prepend(szSearchPath, "/");
    }

    duk_pop(script);

    script.CloseObject();
  }


  ezStringBuilder result;
  TranspileFile(sRequestedFile, result);

  return script.ReturnString(result);
}
