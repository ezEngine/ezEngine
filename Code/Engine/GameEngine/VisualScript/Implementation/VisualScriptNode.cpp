#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode::ezVisualScriptNode() = default;
ezVisualScriptNode::~ezVisualScriptNode() = default;


ezInt32 ezVisualScriptNode::HandlesMessagesWithID() const
{
  return -1;
}

void ezVisualScriptNode::HandleMessage(ezMessage* pMsg) {}

bool ezVisualScriptNode::IsManuallyStepped() const
{
  ezHybridArray<ezAbstractProperty*, 32> properties;
  GetDynamicRTTI()->GetAllProperties(properties);

  for (auto prop : properties)
  {
    if (prop->GetAttributeByType<ezVisScriptExecPinOutAttribute>() != nullptr)
      return true;

    if (prop->GetAttributeByType<ezVisScriptExecPinInAttribute>() != nullptr)
      return true;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVisualScriptDataPinType, 1)
EZ_ENUM_CONSTANTS(ezVisualScriptDataPinType::None, ezVisualScriptDataPinType::Number, ezVisualScriptDataPinType::Boolean, ezVisualScriptDataPinType::Vec3, ezVisualScriptDataPinType::String)
EZ_ENUM_CONSTANTS(ezVisualScriptDataPinType::GameObjectHandle, ezVisualScriptDataPinType::ComponentHandle, ezVisualScriptDataPinType::Variant)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
ezVisualScriptDataPinType::Enum ezVisualScriptDataPinType::GetDataPinTypeForType(const ezRTTI* pType)
{
  auto varType = pType->GetVariantType();
  if (varType >= ezVariant::Type::Int8 && varType <= ezVariant::Type::Double)
  {
    return ezVisualScriptDataPinType::Number;
  }

  switch (varType)
  {
    case ezVariantType::Bool:
      return ezVisualScriptDataPinType::Boolean;

    case ezVariantType::Vector3:
      return ezVisualScriptDataPinType::Vec3;

    case ezVariantType::String:
      return ezVisualScriptDataPinType::String;

    default:
      return pType == ezGetStaticRTTI<ezVariant>() ? ezVisualScriptDataPinType::Variant : ezVisualScriptDataPinType::None;
  }
}

// static
void ezVisualScriptDataPinType::EnforceSupportedType(ezVariant& ref_var)
{
  switch (ref_var.GetType())
  {
    case ezVariantType::Int8:
    case ezVariantType::UInt8:
    case ezVariantType::Int16:
    case ezVariantType::UInt16:
    case ezVariantType::Int32:
    case ezVariantType::UInt32:
    case ezVariantType::Int64:
    case ezVariantType::UInt64:
    case ezVariantType::Float:
    {
      const double value = ref_var.ConvertTo<double>();
      ref_var = value;
      return;
    }

    default:
      return;
  }
}

static ezUInt32 s_StorageSizes[] = {
  ezInvalidIndex,             // None
  sizeof(double),             // Number
  sizeof(bool),               // Boolean
  sizeof(ezVec3),             // Vec3
  sizeof(ezString),           // String
  sizeof(ezGameObjectHandle), // GameObjectHandle
  sizeof(ezComponentHandle),  // ComponentHandle
  sizeof(ezVariant),          // Variant
};

// static
ezUInt32 ezVisualScriptDataPinType::GetStorageByteSize(Enum dataPinType)
{
  if (dataPinType >= Number && dataPinType <= Variant)
  {
    EZ_CHECK_AT_COMPILETIME(EZ_ARRAY_SIZE(s_StorageSizes) == Variant + 1);
    return s_StorageSizes[dataPinType];
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezInvalidIndex;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptExecPinOutAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptExecPinOutAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Slot", m_uiPinSlot)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptExecPinInAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptExecPinInAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Slot", m_uiPinSlot)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptDataPinInAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptDataPinInAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Slot", m_uiPinSlot),
    EZ_ENUM_MEMBER_PROPERTY("Type", ezVisualScriptDataPinType, m_DataType)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptDataPinOutAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptDataPinOutAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Slot", m_uiPinSlot),
    EZ_ENUM_MEMBER_PROPERTY("Type", ezVisualScriptDataPinType, m_DataType)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptNode);
