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

EZ_IMPLEMENT_SINGLETON(ezVisualScriptTypeRegistry);

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, VisualScript)

BEGIN_SUBSYSTEM_DEPENDENCIES
"PluginAssets"
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

  if (const ezColorAttribute* pAttr = pRtti->GetAttributeByType<ezColorAttribute>())
  {
    nd.m_Color = pAttr->GetColor();
  }

  pRtti->GetAllProperties(properties);

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
    }

    if (const ezVisScriptDataPinOutAttribute* pAttr = prop->GetAttributeByType<ezVisScriptDataPinOutAttribute>())
    {
      pd.m_PinType = ezVisualScriptPinDescriptor::Data;
      pd.m_Color = PinTypeColor(pAttr->m_DataType);
      pd.m_DataType = pAttr->m_DataType;
      pd.m_uiPinIndex = pAttr->m_uiPinSlot;
      nd.m_OutputPins.PushBack(pd);
    }

    if (const ezVisScriptExecPinInAttribute* pAttr = prop->GetAttributeByType<ezVisScriptExecPinInAttribute>())
    {
      pd.m_PinType = ezVisualScriptPinDescriptor::Execution;
      pd.m_Color = ExecutionPinColor;
      pd.m_uiPinIndex = pAttr->m_uiPinSlot;
      nd.m_InputPins.PushBack(pd);
    }

    if (const ezVisScriptExecPinOutAttribute* pAttr = prop->GetAttributeByType<ezVisScriptExecPinOutAttribute>())
    {
      pd.m_PinType = ezVisualScriptPinDescriptor::Execution;
      pd.m_Color = ExecutionPinColor;
      pd.m_uiPinIndex = pAttr->m_uiPinSlot;
      nd.m_OutputPins.PushBack(pd);
    }

    if (const ezDefaultValueAttribute* pAttr = prop->GetAttributeByType<ezDefaultValueAttribute>())
    {
      ezVariant val = pAttr->GetValue();

      /// \todo Somehow default values aren't working in the UI
      int i = 0;
    }

    if (prop->GetCategory() == ezPropertyCategory::Constant)
      continue;

    {
      ezReflectedPropertyDescriptor pd;
      pd.m_Flags = prop->GetFlags();
      pd.m_Category = prop->GetCategory();
      pd.m_sName = prop->GetPropertyName();
      pd.m_sType = prop->GetSpecificType()->GetTypeName();
      pd.m_ReferenceAttributes = prop->GetAttributes();

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
