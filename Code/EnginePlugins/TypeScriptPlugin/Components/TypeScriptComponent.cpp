#include <TypeScriptPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezTypeScriptComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Script", GetTypeScriptComponentFile, SetTypeScriptComponentFile)->AddAttributes(new ezAssetBrowserAttribute("TypeScript")),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Script")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Scripting"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezTypeScriptComponent::ezTypeScriptComponent() = default;
ezTypeScriptComponent::~ezTypeScriptComponent() = default;

void ezTypeScriptComponent::SerializeComponent(ezWorldWriter& stream) const
{
  auto& s = stream.GetStream();

  s << m_TypeScriptComponentGuid;

  // version 3
  ezUInt16 uiNumParams = m_Parameters.GetCount();
  s << uiNumParams;

  for (ezUInt32 p = 0; p < uiNumParams; ++p)
  {
    s << m_Parameters.GetKey(p);
    s << m_Parameters.GetValue(p);
  }
}

void ezTypeScriptComponent::DeserializeComponent(ezWorldReader& stream)
{
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  if (uiVersion == 1)
  {
    ezTypelessResourceHandle hDummy;
    s >> hDummy;
  }
  else
  {
    s >> m_TypeScriptComponentGuid;
  }

  if (uiVersion >= 3)
  {
    ezUInt16 uiNumParams = 0;
    s >> uiNumParams;
    m_Parameters.Reserve(uiNumParams);

    ezHashedString key;
    ezVariant value;

    for (ezUInt32 p = 0; p < uiNumParams; ++p)
    {
      s >> key;
      s >> value;

      m_Parameters.Insert(key, value);
    }
  }
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

bool ezTypeScriptComponent::CallTsFunc(const char* szFuncName)
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  ezDuktapeHelper duk(binding.GetDukTapeContext(), 0);

  binding.DukPutComponentObject(this); // [ comp ]

  if (duk.PrepareMethodCall(szFuncName).Succeeded()) // [ comp func comp ]
  {
    duk.CallPreparedMethod(); // [ comp result ]
    duk.PopStack(2);          // [ ]

    return true;
  }
  else
  {
    // remove 'this'   [ comp ]
    duk.PopStack(); // [ ]

    return false;
  }
}

void ezTypeScriptComponent::SetExposedVariables()
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  ezDuktapeHelper duk(binding.GetDukTapeContext(), 0);

  binding.DukPutComponentObject(this); // [ comp ]

  for (ezUInt32 p = 0; p < m_Parameters.GetCount(); ++p)
  {
    const auto& pair = m_Parameters.GetPair(p);

    switch (pair.value.GetType())
    {
      case ezVariantType::Angle:
        duk.SetNumberProperty(pair.key.GetString(), pair.value.Get<ezAngle>().GetRadian());
        break;

      case ezVariantType::Bool:
        duk.SetBoolProperty(pair.key.GetString(), pair.value.Get<bool>());
        break;

      case ezVariantType::Int8:
      case ezVariantType::UInt8:
      case ezVariantType::Int16:
      case ezVariantType::UInt16:
      case ezVariantType::Int32:
      case ezVariantType::UInt32:
      case ezVariantType::Int64:
      case ezVariantType::UInt64:
      case ezVariantType::Float:
      case ezVariantType::Double:
        duk.SetNumberProperty(pair.key.GetString(), pair.value.ConvertTo<double>());
        break;

      case ezVariantType::Color:
        binding.PushColor(duk, pair.value.Get<ezColor>());
        duk.SetCustomProperty(pair.key.GetString());
        break;

      case ezVariantType::ColorGamma:
        binding.PushColor(duk, pair.value.Get<ezColorGammaUB>());
        duk.SetCustomProperty(pair.key.GetString());
        break;

      case ezVariantType::Vector3:
        binding.PushVec3(duk, pair.value.Get<ezVec3>());
        duk.SetCustomProperty(pair.key.GetString());
        break;

      case ezVariantType::Quaternion:
        binding.PushQuat(duk, pair.value.Get<ezQuat>());
        duk.SetCustomProperty(pair.key.GetString());
        break;

      //case ezVariantType::Vector2:
      //case ezVariantType::Vector4:
      //case ezVariantType::Vector2I:
      //case ezVariantType::Vector3I:
      //case ezVariantType::Vector4I:
      //case ezVariantType::Vector2U:
      //case ezVariantType::Vector3U:
      //case ezVariantType::Vector4U:
      //case ezVariantType::Matrix3:
      //case ezVariantType::Matrix4:
      //case ezVariantType::Transform:
      //
      case ezVariantType::String:
        duk.SetStringProperty(pair.key.GetString(), pair.value.Get<ezString>());
        break;

      case ezVariantType::StringView:
      {
        ezStringBuilder tmp;

        duk.SetStringProperty(pair.key.GetString(), pair.value.Get<ezStringView>().GetData(tmp));

        break;
      }

        //case ezVariantType::DataBuffer:
        //case ezVariantType::Time:
        //case ezVariantType::Uuid:

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }


  duk.PopStack(); // [ ]
}

void ezTypeScriptComponent::Initialize()
{
  // SUPER::Initialize() does nothing

  if (!GetWorld()->GetWorldSimulationEnabled())
    return;

  if (!GetUserFlag(UserFlag::InitializedTS))
  {
    SetUserFlag(UserFlag::InitializedTS, true);

    CallTsFunc("Initialize");
  }
}

void ezTypeScriptComponent::Deinitialize()
{
  // mirror what ezComponent::Deinitialize does, but make sure to CallTsFunc at the right time

  EZ_ASSERT_DEV(GetOwner() != nullptr, "Owner must still be valid");

  if (IsActive())
  {
    SetActive(false);
  }

  if (GetWorld()->GetWorldSimulationEnabled() && GetUserFlag(UserFlag::InitializedTS))
  {
    CallTsFunc("Deinitialize");
  }

  SetUserFlag(UserFlag::InitializedTS, false);

  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();
  binding.DeleteTsComponent(GetHandle());
}

void ezTypeScriptComponent::OnActivated()
{
  // SUPER::OnActivated() does nothing

  if (!GetWorld()->GetWorldSimulationEnabled())
    return;

  ezTypeScriptComponent::Initialize();

  SetUserFlag(UserFlag::OnActivatedTS, true);

  CallTsFunc("OnActivated");
}

void ezTypeScriptComponent::OnDeactivated()
{
  if (GetWorld()->GetWorldSimulationEnabled() && GetUserFlag(UserFlag::OnActivatedTS))
  {
    CallTsFunc("OnDeactivated");
  }

  SetUserFlag(UserFlag::OnActivatedTS, false);

  // SUPER::OnDeactivated() does nothing
}

void ezTypeScriptComponent::OnSimulationStarted()
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  if (binding.LoadComponent(m_TypeScriptComponentGuid, m_ComponentTypeInfo).Succeeded())
  {
    ezUInt32 uiStashIdx = 0;
    binding.RegisterComponent(m_ComponentTypeInfo.Value().m_sComponentTypeName, GetHandle(), uiStashIdx);

    // if the TS component has any message handlers, we need to capture all messages and redirect them to the script
    EnableUnhandledMessageHandler(!m_ComponentTypeInfo.Value().m_MessageHandlers.IsEmpty());

    SetExposedVariables();
  }

  ezTypeScriptComponent::OnActivated();

  CallTsFunc("OnSimulationStarted");
}

void ezTypeScriptComponent::Update(ezTypeScriptBinding& binding)
{
  if (GetUserFlag(UserFlag::NoTsTick))
    return;

  if (!CallTsFunc("Tick"))
  {
    SetUserFlag(UserFlag::NoTsTick, true);
  }
}

void ezTypeScriptComponent::SetTypeScriptComponentFile(const char* szFile)
{
  if (ezConversionUtils::IsStringUuid(szFile))
  {
    SetTypeScriptComponentGuid(ezConversionUtils::ConvertStringToUuid(szFile));
  }
  else
  {
    SetTypeScriptComponentGuid(ezUuid());
  }
}

const char* ezTypeScriptComponent::GetTypeScriptComponentFile() const
{
  if (m_TypeScriptComponentGuid.IsValid())
  {
    static ezStringBuilder sGuid; // need dummy storage
    return ezConversionUtils::ToString(m_TypeScriptComponentGuid, sGuid);
  }

  return "";
}

void ezTypeScriptComponent::SetTypeScriptComponentGuid(const ezUuid& hResource)
{
  m_TypeScriptComponentGuid = hResource;
}

const ezUuid& ezTypeScriptComponent::GetTypeScriptComponentGuid() const
{
  return m_TypeScriptComponentGuid;
}


const ezRangeView<const char*, ezUInt32> ezTypeScriptComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32 { return 0; }, [this]() -> ezUInt32 { return m_Parameters.GetCount(); },
    [](ezUInt32& it) { ++it; }, [this](const ezUInt32& it) -> const char* { return m_Parameters.GetKey(it).GetString().GetData(); });
}

void ezTypeScriptComponent::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != ezInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  //GetWorld()->GetComponentManager<ezTypeScriptComponentManager>()->AddToUpdateList(this);
}

void ezTypeScriptComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(ezTempHashedString(szKey)))
  {
    //GetWorld()->GetComponentManager<ezTypeScriptComponentManager>()->AddToUpdateList(this);
  }
}

bool ezTypeScriptComponent::GetParameter(const char* szKey, ezVariant& out_value) const
{
  ezUInt32 it = m_Parameters.Find(szKey);

  if (it == ezInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}
