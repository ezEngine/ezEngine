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

void ezTypeScriptComponent::Update(ezTypeScriptWrapper& TsWrapper)
{
  ezDuktapeWrapper& script = TsWrapper.m_Script;

  script.OpenGlobalStashObject();

  const int iOwnRef = (int)GetHandle().GetInternalID().m_Data;
  duk_push_int(script.GetContext(), iOwnRef);
  duk_get_prop(script.GetContext(), -2);

  {
    if (script.BeginFunctionCall("Update").Succeeded())
    {
      duk_dup(script.GetContext(), -2); // this

      duk_call_method(script.GetContext(), 0);
      script.EndFunctionCall();
    }
  }

  duk_pop(script.GetContext());
  script.CloseObject();
}
