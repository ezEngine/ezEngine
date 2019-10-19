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
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->m_TsBinding;

  if (binding.LoadComponent("TypeScript/MyComponent.ts").Succeeded())
  {
    ezUInt32 uiStashIdx = 0;
    binding.RegisterComponent("MyComponent", GetHandle(), uiStashIdx);
  }

  EnableUnhandledMessageHandler(true);
}

void ezTypeScriptComponent::Deinitialize()
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->m_TsBinding;
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
  ezTypeScriptBinding& binding = const_cast<ezTypeScriptBinding&>(static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->m_TsBinding);

  ezStringBuilder sMsgName = msg.GetDynamicRTTI()->GetTypeName();
  sMsgName.TrimWordStart("ez");

  if (sMsgName != "MsgSetColor" && sMsgName != "MsgSetFloatParameter")
    return false;

  ezStringBuilder sFuncName("On", sMsgName);

  ezDuktapeHelper duk(binding.GetDukTapeContext(), 0);

  binding.DukPutComponentObject(this); // [ comp ]

  if (duk.PrepareMethodCall(sFuncName).Succeeded()) // [ comp func comp ]
  {
    ezTypeScriptBinding::DukPutMessage(duk, msg); // [ comp func comp msg ]
    duk.PushCustom();                             // [ comp func comp msg ]
    duk.CallPreparedMethod();                     // [ comp result ]
    duk.PopStack(2);                              // [ ]

    return true;
  }
  else
  {
    // remove 'this'   [ comp ]
    duk.PopStack(); // [ ]

    return false;
  }
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
