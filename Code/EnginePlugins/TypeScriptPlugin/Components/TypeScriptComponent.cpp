#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezTypeScriptComponent, 1, ezComponentMode::Static)
{
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezTypeScriptComponent::ezTypeScriptComponent()
{
}

ezTypeScriptComponent::~ezTypeScriptComponent()
{
  // TODO: remove reference from stash
}

void ezTypeScriptComponent::SerializeComponent(ezWorldWriter& stream) const
{
}

void ezTypeScriptComponent::DeserializeComponent(ezWorldReader& stream)
{
}

int TS_ezLog_Info(duk_context* pContext);

void ezTypeScriptComponent::OnSimulationStarted()
{
  auto& script = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->m_Script;

  if (!m_bScriptLoaded)
  {
    m_bScriptLoaded = true;

    ezStringBuilder js;
    if (TranspileFile("TypeScript/Component.ts", js).Failed())
      return;

    if (script.ExecuteString(js, "Component.ts").Failed())
      return;

    if (script.OpenObject("ezLog").Succeeded())
    {
      script.RegisterFunction("Error", TS_ezLog_Info, 1, ezLogMsgType::ErrorMsg);
      script.RegisterFunction("SeriousWarning", TS_ezLog_Info, 1, ezLogMsgType::SeriousWarningMsg);
      script.RegisterFunction("Warning", TS_ezLog_Info, 1, ezLogMsgType::WarningMsg);
      script.RegisterFunction("Success", TS_ezLog_Info, 1, ezLogMsgType::SuccessMsg);
      script.RegisterFunction("Info", TS_ezLog_Info, 1, ezLogMsgType::InfoMsg);
      script.RegisterFunction("Dev", TS_ezLog_Info, 1, ezLogMsgType::DevMsg);
      script.RegisterFunction("Debug", TS_ezLog_Info, 1, ezLogMsgType::DebugMsg);
      script.CloseObject();
    }
  }

  duk_push_global_stash(script.GetContext());

  if (script.BeginFunctionCall("_Create_MyComponent").Succeeded())
  {
    script.PushParameter(GetOwner()->GetName());
    script.ExecuteFunctionCall();

    // store a back pointer in the object
    {
      // TODO: store a handle instead
      duk_push_pointer(script.GetContext(), this);
      duk_put_prop_string(script.GetContext(), -2, "ezComponentPtr");
    }

    const int iOwnRef = (int)GetHandle().GetInternalID().m_Data;
    duk_push_int(script.GetContext(), iOwnRef);
    duk_dup(script.GetContext(), -2);
    duk_put_prop(script.GetContext(), -4);

    script.EndFunctionCall();
  }

  duk_pop(script.GetContext());
}

void ezTypeScriptComponent::Update(ezDuktapeWrapper& script)
{
  script.OpenGlobalStashObject();

  const int iOwnRef = (int)GetHandle().GetInternalID().m_Data;
  duk_push_int(script.GetContext(), iOwnRef);
  duk_get_prop(script.GetContext(), -2);

  //if (script.OpenObject("MyTsComponent").Succeeded())
  {
    if (script.BeginFunctionCall("Update").Succeeded())
    {
      duk_dup(script.GetContext(), -2); // this

      //script.ExecuteFunctionCall();
      duk_call_method(script.GetContext(), 0);
      script.EndFunctionCall();
    }

    //script.CloseObject();
  }
  duk_pop(script.GetContext());
  script.CloseObject();
}

ezResult ezTypeScriptComponent::TranspileFile(const char* szFile, ezStringBuilder& result) const
{
  static ezDuktapeWrapper s_TranspilerScript("TsTranspiler");
  static bool s_bTranspilerLoaded = false;

  if (!s_bTranspilerLoaded)
  {
    s_bTranspilerLoaded = true;

    EZ_VERIFY(s_TranspilerScript.ExecuteFile("TypeScript/typescriptServices.js").Succeeded(), "");
  }

  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  result.ReadAll(file);

  EZ_SUCCEED_OR_RETURN(s_TranspilerScript.OpenObject("ts"));
  EZ_SUCCEED_OR_RETURN(s_TranspilerScript.BeginFunctionCall("transpile"));
  s_TranspilerScript.PushParameter(result);
  EZ_SUCCEED_OR_RETURN(s_TranspilerScript.ExecuteFunctionCall());

  result = s_TranspilerScript.GetStringReturnValue();

  s_TranspilerScript.EndFunctionCall();
  s_TranspilerScript.CloseObject();

  ezStringBuilder sOutFile = szFile;
  sOutFile.ChangeFileExtension("js");
  sOutFile.Prepend(":project/");

  ezFileWriter fileOut;
  fileOut.Open(sOutFile);
  fileOut.WriteBytes(result.GetData(), result.GetElementCount());

  return EZ_SUCCESS;
}
