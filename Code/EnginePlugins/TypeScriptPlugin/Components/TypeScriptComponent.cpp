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
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->m_TsBinding;
  ezDuktapeWrapper& duk = binding.GetDukTapeWrapper();

  binding.SetupBinding();

  ezDuktapeStackValidator validator(duk);

  duk_push_global_stash(duk);

  if (duk.BeginFunctionCall("__Ts_Create_MyComponent").Succeeded())
  {
    duk.PushParameter(GetOwner()->GetName());
    EZ_ASSERT_DEV(duk.ExecuteFunctionCall().Succeeded(), "");

    // store a back pointer in the object
    {
      // TODO: store a handle instead
      duk_push_pointer(duk, this);
      duk_put_prop_string(duk, -2, "ezComponentPtr");
    }

    const int iOwnRef = (int)GetHandle().GetInternalID().m_Data;
    duk_push_int(duk, iOwnRef);
    duk_dup(duk, -2);
    duk_put_prop(duk, -4);

    duk.EndFunctionCall();
  }

  duk_pop(duk);
}

void ezTypeScriptComponent::Update(ezTypeScriptBinding& binding)
{
  ezDuktapeWrapper& duk = binding.GetDukTapeWrapper();

  ezDuktapeStackValidator validator(duk);

  duk.OpenGlobalStashObject();

  const int iOwnRef = (int)GetHandle().GetInternalID().m_Data;
  duk_push_int(duk, iOwnRef);
  duk_get_prop(duk, -2);

  {
    if (duk.BeginFunctionCall("Update").Succeeded())
    {
      duk_dup(duk, -2); // this

      duk_call_method(duk, 0);
      duk.EndFunctionCall();
    }
  }

  duk_pop(duk);
  duk.CloseObject();
}
