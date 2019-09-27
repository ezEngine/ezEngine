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

  if (binding.LoadComponent("TypeScript/Component.ts").Succeeded())
  {
    ezDuktapeStackValidator validator(duk);

    // TODO: add 'ezTypeScriptBinding::CreateComponent(name)' function
    if (duk.BeginFunctionCall("__TS_Create_MyComponent").Succeeded())
    {
      duk.PushParameter(GetOwner()->GetName());
      EZ_ASSERT_DEV(duk.ExecuteFunctionCall().Succeeded(), "");

      // store own handle in obj as property
      ezComponentHandle hOwnHandle = GetHandle();
      ezComponentHandle* pBuffer = reinterpret_cast<ezComponentHandle*>(duk_push_fixed_buffer(duk, sizeof(ezComponentHandle)));
      *pBuffer = hOwnHandle;
      duk_put_prop_index(duk, -2, ezTypeScriptBindingIndexProperty::ComponentHandle);

      {
        const ezUInt32 uiOwnReference = hOwnHandle.GetInternalID().m_Data;

        duk.OpenGlobalStashObject();
        duk_push_uint(duk, uiOwnReference);
        duk_dup(duk, -3); // duplicate component obj
        duk_put_prop(duk, -3);
        duk.CloseObject();
      }

      duk.EndFunctionCall();
    }
  }
}

void ezTypeScriptComponent::Update(ezTypeScriptBinding& binding)
{
  ezDuktapeWrapper& duk = binding.GetDukTapeWrapper();

  ezDuktapeStackValidator validator(duk);

  // TODO: add 'ezTypeScriptBinding::DukPutComponent(handle)'
  duk.OpenGlobalStashObject();

  const ezUInt32 uiOwnHandle = GetHandle().GetInternalID().m_Data;
  duk_push_uint(duk, uiOwnHandle);
  duk_get_prop(duk, -2);

  {
    // TODO: add 'BeginMethodCall'
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
