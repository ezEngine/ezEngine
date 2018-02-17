#include <PCH.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetManager.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraphQt.moc.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <Core/World/Component.h>
#include <Core/Messages/ScriptFunctionMessage.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

EZ_IMPLEMENT_SINGLETON(ezVisualScriptTypeRegistry);

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, VisualScript)

BEGIN_SUBSYSTEM_DEPENDENCIES
"PluginAssets", "ReflectedTypeManager"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  EZ_DEFAULT_NEW(ezVisualScriptTypeRegistry);

  ezVisualScriptTypeRegistry::GetSingleton()->UpdateNodeTypes();
  const ezRTTI* pBaseType = ezVisualScriptTypeRegistry::GetSingleton()->GetNodeBaseType();

  ezQtNodeScene::GetPinFactory().RegisterCreator(ezGetStaticRTTI<ezVisualScriptPin>(), [](const ezRTTI* pRtti)->ezQtPin* { return new ezQtVisualScriptPin(); });
  ezQtNodeScene::GetConnectionFactory().RegisterCreator(ezGetStaticRTTI<ezVisualScriptConnection>(), [](const ezRTTI* pRtti)->ezQtConnection* { return new ezQtVisualScriptConnection(); });
  ezQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const ezRTTI* pRtti)->ezQtNode* { return new ezQtVisualScriptNode(); });
}

ON_CORE_SHUTDOWN
{
  ezVisualScriptTypeRegistry* pDummy = ezVisualScriptTypeRegistry::GetSingleton();
  EZ_DEFAULT_DELETE(pDummy);
}

ON_ENGINE_STARTUP
{
}

ON_ENGINE_SHUTDOWN
{
}

EZ_END_SUBSYSTEM_DECLARATION

ezVisualScriptTypeRegistry::ezVisualScriptTypeRegistry()
  : m_SingletonRegistrar(this)
{
  m_pBaseType = nullptr;
}

const ezVisualScriptNodeDescriptor* ezVisualScriptTypeRegistry::GetDescriptorForType(const ezRTTI* pRtti) const
{
  auto it = m_NodeDescriptors.Find(pRtti);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}

void ezVisualScriptTypeRegistry::PhantomTypeRegistryEventHandler(const ezPhantomRttiManagerEvent& e)
{
  if (e.m_Type == ezPhantomRttiManagerEvent::Type::TypeAdded || e.m_Type == ezPhantomRttiManagerEvent::Type::TypeChanged)
  {
    UpdateNodeType(e.m_pChangedType);
  }
}

void ezVisualScriptTypeRegistry::UpdateNodeTypes()
{
  // Base Node Type
  if (m_pBaseType == nullptr)
  {
    ezReflectedTypeDescriptor desc;
    desc.m_sTypeName = "ezVisualScriptNodeBase";
    desc.m_sPluginName = "VisualScriptTypes";
    desc.m_sParentTypeName = ezGetStaticRTTI<ezReflectedClass>()->GetTypeName();
    desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Abstract | ezTypeFlags::Class;
    desc.m_uiTypeSize = 0;
    desc.m_uiTypeVersion = 1;

    m_pBaseType = ezPhantomRttiManager::RegisterType(desc);

    ezPhantomRttiManager::s_Events.AddEventHandler(ezMakeDelegate(&ezVisualScriptTypeRegistry::PhantomTypeRegistryEventHandler, this));
  }

  auto& dynEnum = ezDynamicStringEnum::GetDynamicEnum("ComponentTypes");
  dynEnum.Clear();

  for (const ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (pRtti->IsDerivedFrom<ezComponent>())
    {
      dynEnum.AddValidValue(pRtti->GetTypeName(), true);
    }
  }

  for (const ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    UpdateNodeType(pRtti);
  }
}

static ezColor PinTypeColor(ezVisualScriptDataPinType::Enum type)
{
  switch (type)
  {
  case ezVisualScriptDataPinType::Number:
    return ezColor::DarkGoldenRod;
  case ezVisualScriptDataPinType::Boolean:
    return ezColor::DarkGreen;
  case ezVisualScriptDataPinType::Vec3:
    return ezColor::Gold;
  case ezVisualScriptDataPinType::GameObjectHandle:
    return ezColor::Maroon;
  case ezVisualScriptDataPinType::ComponentHandle:
    return ezColor::DodgerBlue;
  }

  return ezColor::Pink;
}

void ezVisualScriptTypeRegistry::UpdateNodeType(const ezRTTI* pRtti)
{
  if (pRtti->GetAttributeByType<ezHiddenAttribute>() != nullptr)
    return;

  if (pRtti->IsDerivedFrom<ezScriptFunctionMessage>() && pRtti != ezGetStaticRTTI<ezScriptFunctionMessage>())
  {
    CreateMessageNodeType(pRtti);
    return;
  }

  if (!pRtti->IsDerivedFrom<ezVisualScriptNode>() || pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract))
    return;

  ezHybridArray<ezAbstractProperty*, 32> properties;
  static const ezColor ExecutionPinColor = ezColor::LightSlateGrey;

  ezVisualScriptNodeDescriptor nd;
  nd.m_sTypeName = pRtti->GetTypeName();
  nd.m_Color = ezColor::CadetBlue;

  if (const ezCategoryAttribute* pAttr = pRtti->GetAttributeByType<ezCategoryAttribute>())
  {
    nd.m_sCategory = pAttr->GetCategory();

    if (nd.m_sCategory == "Debug")
      nd.m_Color = ezColorGammaUB(127, 0, 110);
    else if (nd.m_sCategory == "Logic")
      nd.m_Color = ezColorGammaUB(128, 0, 0);
    else if (nd.m_sCategory == "Input")
      nd.m_Color = ezColorGammaUB(38, 105, 0);
    else if (nd.m_sCategory == "Math")
      nd.m_Color = ezColorGammaUB(183, 153, 0);
    else if (nd.m_sCategory == "Objects")
      nd.m_Color = ezColorGammaUB(0, 53, 91);
    else if (nd.m_sCategory == "Components")
      nd.m_Color = ezColorGammaUB(0, 53, 91);
    else if (nd.m_sCategory == "References")
      nd.m_Color = ezColorGammaUB(0, 89, 153);
    else if (nd.m_sCategory == "Variables")
      nd.m_Color = ezColorGammaUB(250, 70, 0);
  }

  if (const ezTitleAttribute* pAttr = pRtti->GetAttributeByType<ezTitleAttribute>())
  {
    nd.m_sTitle = pAttr->GetTitle();
  }

  if (const ezColorAttribute* pAttr = pRtti->GetAttributeByType<ezColorAttribute>())
  {
    nd.m_Color = pAttr->GetColor();
  }

  pRtti->GetAllProperties(properties);

  ezSet<ezInt32> usedInputDataPinIDs;
  ezSet<ezInt32> usedOutputDataPinIDs;
  ezSet<ezInt32> usedInputExecPinIDs;
  ezSet<ezInt32> usedOutputExecPinIDs;

  for (auto prop : properties)
  {
    ezVisualScriptPinDescriptor pd;
    pd.m_sName = prop->GetPropertyName();
    pd.m_sTooltip = ""; /// \todo Use ezTranslateTooltip

    if (const ezVisScriptDataPinInAttribute* pAttr = prop->GetAttributeByType<ezVisScriptDataPinInAttribute>())
    {
      pd.m_PinType = ezVisualScriptPinDescriptor::Data;
      pd.m_Color = PinTypeColor(pAttr->m_DataType);
      pd.m_DataType = pAttr->m_DataType;
      pd.m_uiPinIndex = pAttr->m_uiPinSlot;
      nd.m_InputPins.PushBack(pd);

      if (usedInputDataPinIDs.Contains(pd.m_uiPinIndex))
        ezLog::Error("Visual Script Node '{0}' uses the same input data pin index multiple times: '{1}'", nd.m_sTypeName, pd.m_uiPinIndex);

      usedInputDataPinIDs.Insert(pd.m_uiPinIndex);
    }

    if (const ezVisScriptDataPinOutAttribute* pAttr = prop->GetAttributeByType<ezVisScriptDataPinOutAttribute>())
    {
      pd.m_PinType = ezVisualScriptPinDescriptor::Data;
      pd.m_Color = PinTypeColor(pAttr->m_DataType);
      pd.m_DataType = pAttr->m_DataType;
      pd.m_uiPinIndex = pAttr->m_uiPinSlot;
      nd.m_OutputPins.PushBack(pd);

      if (usedOutputDataPinIDs.Contains(pd.m_uiPinIndex))
        ezLog::Error("Visual Script Node '{0}' uses the same output data pin index multiple times: '{1}'", nd.m_sTypeName, pd.m_uiPinIndex);

      usedOutputDataPinIDs.Insert(pd.m_uiPinIndex);
    }

    if (const ezVisScriptExecPinInAttribute* pAttr = prop->GetAttributeByType<ezVisScriptExecPinInAttribute>())
    {
      pd.m_PinType = ezVisualScriptPinDescriptor::Execution;
      pd.m_Color = ExecutionPinColor;
      pd.m_uiPinIndex = pAttr->m_uiPinSlot;
      nd.m_InputPins.PushBack(pd);

      if (usedInputExecPinIDs.Contains(pd.m_uiPinIndex))
        ezLog::Error("Visual Script Node '{0}' uses the same input exec pin index multiple times: '{1}'", nd.m_sTypeName, pd.m_uiPinIndex);

      usedInputExecPinIDs.Insert(pd.m_uiPinIndex);
    }

    if (const ezVisScriptExecPinOutAttribute* pAttr = prop->GetAttributeByType<ezVisScriptExecPinOutAttribute>())
    {
      pd.m_PinType = ezVisualScriptPinDescriptor::Execution;
      pd.m_Color = ExecutionPinColor;
      pd.m_uiPinIndex = pAttr->m_uiPinSlot;
      nd.m_OutputPins.PushBack(pd);

      if (usedOutputExecPinIDs.Contains(pd.m_uiPinIndex))
        ezLog::Error("Visual Script Node '{0}' uses the same output exec pin index multiple times: '{1}'", nd.m_sTypeName, pd.m_uiPinIndex);

      usedOutputExecPinIDs.Insert(pd.m_uiPinIndex);
    }

    if (prop->GetCategory() == ezPropertyCategory::Constant)
      continue;

    {
      ezReflectedPropertyDescriptor pd;
      pd.m_Flags = prop->GetFlags();
      pd.m_Category = prop->GetCategory();
      pd.m_sName = prop->GetPropertyName();
      pd.m_sType = prop->GetSpecificType()->GetTypeName();

      for (ezPropertyAttribute* const pAttr : prop->GetAttributes())
      {
        pd.m_Attributes.PushBack(ezReflectionSerializer::Clone(pAttr));
      }

      nd.m_Properties.PushBack(pd);
    }
  }

  m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);
}

const ezRTTI* ezVisualScriptTypeRegistry::GenerateTypeFromDesc(const ezVisualScriptNodeDescriptor& nd)
{
  ezStringBuilder temp;
  temp.Set("VisualScriptNode::", nd.m_sTypeName);

  ezReflectedTypeDescriptor desc;
  desc.m_sTypeName = temp;
  desc.m_sPluginName = "VisualScriptTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Class;
  desc.m_uiTypeSize = 0;
  desc.m_uiTypeVersion = 1;
  desc.m_Properties = nd.m_Properties;

  return ezPhantomRttiManager::RegisterType(desc);
}

void ezVisualScriptTypeRegistry::CreateMessageNodeType(const ezRTTI* pRtti)
{
  ezVisualScriptNodeDescriptor nd;
  nd.m_sTypeName = pRtti->GetTypeName();
  nd.m_Color = ezColor::MediumPurple;
  nd.m_sCategory = "Messages";

  if (const ezCategoryAttribute* pAttr = pRtti->GetAttributeByType<ezCategoryAttribute>())
  {
    nd.m_sCategory = pAttr->GetCategory();
  }

  if (const ezColorAttribute* pAttr = pRtti->GetAttributeByType<ezColorAttribute>())
  {
    nd.m_Color = pAttr->GetColor();
  }

  ezHybridArray<ezAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  // Add an input execution pin
  {
    static const ezColor ExecutionPinColor = ezColor::LightSlateGrey;

    ezVisualScriptPinDescriptor pd;
    pd.m_sName = "send";
    pd.m_sTooltip = "When executed, the message is sent to the object or component.";
    pd.m_PinType = ezVisualScriptPinDescriptor::Execution;
    pd.m_Color = ExecutionPinColor;
    pd.m_uiPinIndex = 0;
    nd.m_InputPins.PushBack(pd);
  }

  // Add an output execution pin
  {
    static const ezColor ExecutionPinColor = ezColor::LightSlateGrey;

    ezVisualScriptPinDescriptor pd;
    pd.m_sName = "then";
    pd.m_sTooltip = "";
    pd.m_PinType = ezVisualScriptPinDescriptor::Execution;
    pd.m_Color = ExecutionPinColor;
    pd.m_uiPinIndex = 0;
    nd.m_OutputPins.PushBack(pd);
  }

  // Add an input data pin for the target object
  {
    ezVisualScriptPinDescriptor pd;
    pd.m_sName = "Object";
    pd.m_sTooltip = "When the object is given, the message is sent to all its components.";
    pd.m_PinType = ezVisualScriptPinDescriptor::Data;
    pd.m_DataType = ezVisualScriptDataPinType::GameObjectHandle;
    pd.m_Color = PinTypeColor(ezVisualScriptDataPinType::GameObjectHandle);
    pd.m_uiPinIndex = 0;
    nd.m_InputPins.PushBack(pd);
  }

  // Add an input data pin for the target component
  {
    ezVisualScriptPinDescriptor pd;
    pd.m_sName = "Component";
    pd.m_sTooltip = "When the component is given, the message is sent directly to it.";
    pd.m_PinType = ezVisualScriptPinDescriptor::Data;
    pd.m_DataType = ezVisualScriptDataPinType::ComponentHandle;
    pd.m_Color = PinTypeColor(ezVisualScriptDataPinType::ComponentHandle);
    pd.m_uiPinIndex = 1;
    nd.m_InputPins.PushBack(pd);
  }

  // Delayed Delivery Property
  {
    ezReflectedPropertyDescriptor prd;
    prd.m_Flags = ezPropertyFlags::StandardType | ezPropertyFlags::Phantom;
    prd.m_Category = ezPropertyCategory::Member;
    prd.m_sName = "Delay";
    prd.m_sType = ezGetStaticRTTI<ezTime>()->GetTypeName();

    nd.m_Properties.PushBack(prd);
  }

  // Recursive Delivery Property
  {
    ezReflectedPropertyDescriptor prd;
    prd.m_Flags = ezPropertyFlags::StandardType | ezPropertyFlags::Phantom;
    prd.m_Category = ezPropertyCategory::Member;
    prd.m_sName = "Recursive";
    prd.m_sType = ezGetStaticRTTI<bool>()->GetTypeName();

    nd.m_Properties.PushBack(prd);
  }

  ezInt32 iDataPinIndex = 1; // the first valid index is '2', because of the object and component data pins
  for (auto prop : properties)
  {
    if (prop->GetCategory() == ezPropertyCategory::Constant)
      continue;

    {
      ezReflectedPropertyDescriptor prd;
      prd.m_Flags = prop->GetFlags();
      prd.m_Category = prop->GetCategory();
      prd.m_sName = prop->GetPropertyName();
      prd.m_sType = prop->GetSpecificType()->GetTypeName();

      for (ezPropertyAttribute* const pAttr : prop->GetAttributes())
      {
        prd.m_Attributes.PushBack(ezReflectionSerializer::Clone(pAttr));
      }

      nd.m_Properties.PushBack(prd);
    }

    ++iDataPinIndex;
    const auto varType = prop->GetSpecificType()->GetVariantType();
    if (varType != ezVariantType::Bool && varType != ezVariantType::Double && varType != ezVariantType::Vector3)
      continue;

    ezVisualScriptPinDescriptor pid;

    switch (varType)
    {
    case ezVariantType::Bool:
      pid.m_DataType = ezVisualScriptDataPinType::Boolean;
      break;
    case ezVariantType::Double:
      pid.m_DataType = ezVisualScriptDataPinType::Number;
      break;
    case ezVariantType::Vector3:
      pid.m_DataType = ezVisualScriptDataPinType::Vec3;
      break;
    }

    pid.m_sName = prop->GetPropertyName();
    pid.m_sTooltip = ""; /// \todo Use ezTranslateTooltip
    pid.m_PinType = ezVisualScriptPinDescriptor::Data;
    pid.m_Color = PinTypeColor(pid.m_DataType);
    pid.m_uiPinIndex = iDataPinIndex;
    nd.m_InputPins.PushBack(pid);
  }

  m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);
}
