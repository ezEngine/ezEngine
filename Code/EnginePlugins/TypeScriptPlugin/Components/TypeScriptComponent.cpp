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

ezTypeScriptComponent::ezTypeScriptComponent() = default;
ezTypeScriptComponent::~ezTypeScriptComponent() = default;

void ezTypeScriptComponent::SerializeComponent(ezWorldWriter& stream) const
{
}

void ezTypeScriptComponent::DeserializeComponent(ezWorldReader& stream)
{
}

void ezTypeScriptComponent::OnSimulationStarted()
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->m_TsBinding;

  if (binding.LoadComponent("TypeScript/Component.ts").Succeeded())
  {
    binding.CreateTsComponent("MyComponent", GetHandle(), GetOwner()->GetName());
  }
}

void ezTypeScriptComponent::Deinitialize()
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->m_TsBinding;
  binding.DeleteTsComponent(GetHandle());
}

void ezTypeScriptComponent::Update(ezTypeScriptBinding& binding)
{
  ezDuktapeWrapper& duk = binding.GetDukTapeWrapper();

  ezDuktapeStackValidator validator(duk);

  binding.DukPutComponentObject(GetHandle());

  if (duk.BeginMethodCall("Update").Succeeded())
  {
    duk.ExecuteMethodCall();
    duk.EndMethodCall();
  }

  // remove 'this'
  duk_pop(duk);
}
