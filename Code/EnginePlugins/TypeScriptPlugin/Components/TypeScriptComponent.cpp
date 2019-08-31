#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileReader.h>
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
}

void ezTypeScriptComponent::SerializeComponent(ezWorldWriter& stream) const
{
}

void ezTypeScriptComponent::DeserializeComponent(ezWorldReader& stream)
{
}

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
  }

  duk_push_global_stash(script.GetContext());

  if (script.BeginFunctionCall("_Create_MyComponent").Succeeded())
  {
    script.PushParameter(GetOwner()->GetName());
    script.ExecuteFunctionCall();

    //duk_put_prop_string(script.GetContext(), -3, "MyTsComponent");
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

  return EZ_SUCCESS;
}
