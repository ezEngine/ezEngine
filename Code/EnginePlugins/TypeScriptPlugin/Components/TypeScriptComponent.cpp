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

void ezTypeScriptComponent::OnSimulationStarted()
{
  auto& TsWrapper = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->m_TsWrapper;
  auto& script = TsWrapper.m_Script;

  TsWrapper.SetupScript();

  ezDuktapeStackValidator validator(script);

  duk_push_global_stash(script);

  if (script.BeginFunctionCall("__Ts_Create_MyComponent").Succeeded())
  {
    script.PushParameter(GetOwner()->GetName());
    EZ_ASSERT_DEV(script.ExecuteFunctionCall().Succeeded(), "");

    // store a back pointer in the object
    {
      // TODO: store a handle instead
      duk_push_pointer(script, this);
      duk_put_prop_string(script, -2, "ezComponentPtr");
    }

    const int iOwnRef = (int)GetHandle().GetInternalID().m_Data;
    duk_push_int(script, iOwnRef);
    duk_dup(script, -2);
    duk_put_prop(script, -4);

    script.EndFunctionCall();
  }

  duk_pop(script);
}

void ezTypeScriptComponent::Update(ezTypeScriptWrapper& TsWrapper)
{
  ezDuktapeWrapper& script = TsWrapper.m_Script;

  ezDuktapeStackValidator validator(script);

  script.OpenGlobalStashObject();

  const int iOwnRef = (int)GetHandle().GetInternalID().m_Data;
  duk_push_int(script, iOwnRef);
  duk_get_prop(script, -2);

  {
    if (script.BeginFunctionCall("Update").Succeeded())
    {
      duk_dup(script, -2); // this

      duk_call_method(script, 0);
      script.EndFunctionCall();
    }
  }

  duk_pop(script);
  script.CloseObject();
}
