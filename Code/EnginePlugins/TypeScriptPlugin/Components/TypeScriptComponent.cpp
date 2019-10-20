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

ezTypeScriptTranspiler ezTypeScriptComponentManager::s_Transpiler;

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
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  if (binding.LoadComponent("MyComponent", m_ComponentTypeInfo).Succeeded())
  {
    ezUInt32 uiStashIdx = 0;
    binding.RegisterComponent(m_ComponentTypeInfo.Key(), GetHandle(), uiStashIdx);
  }

  // if the TS component has any message handlers, we need to capture all messages and redirect them to the script
  EnableUnhandledMessageHandler(!m_ComponentTypeInfo.Value().m_MessageHandlers.IsEmpty());
}

void ezTypeScriptComponent::Deinitialize()
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();
  binding.DeleteTsComponent(GetHandle());
}

bool ezTypeScriptComponent::OnUnhandledMessage(ezMessage& msg)
{
  return HandleUnhandledMessage(msg);
}

bool ezTypeScriptComponent::OnUnhandledMessage(ezMessage& msg) const
{
  return const_cast<ezTypeScriptComponent*>(this)->HandleUnhandledMessage(msg);
}

bool ezTypeScriptComponent::HandleUnhandledMessage(ezMessage& msg)
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  return binding.DeliverMessage(m_ComponentTypeInfo, this, msg);
}

void ezTypeScriptComponent::Update(ezTypeScriptBinding& binding)
{
  ezDuktapeHelper duk(binding.GetDukTapeContext(), 0);

  binding.DukPutComponentObject(this); // [ comp ]

  if (duk.PrepareMethodCall("Update").Succeeded()) // [ comp Update comp ]
  {
    duk.CallPreparedMethod(); // [ comp result ]
    duk.PopStack(2);          // [ ]
  }
  else
  {
    // remove 'this'   [ comp ]
    duk.PopStack(); // [ ]
  }
}
