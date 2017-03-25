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

  ezVisualScriptTypeRegistry::GetSingleton()->LoadNodeData();
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

void ezVisualScriptTypeRegistry::LoadNodeData()
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
  }

  UpdateNodeData();
}

void ezVisualScriptTypeRegistry::UpdateNodeData()
{
  ezHybridArray<ezAbstractProperty*, 32> properties;

  for (const ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<ezVisualScriptNode>() || pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract))
      continue;

    ezVisualScriptNodeDescriptor nd;
    nd.m_sTypeName = pRtti->GetTypeName();
    nd.m_Color = ezColor::CadetBlue;

    if (const ezCategoryAttribute* pAttr = pRtti->GetAttributeByType<ezCategoryAttribute>())
    {
      nd.m_sCategory = pAttr->GetCategory();

      //if (nd.m_sCategory == "stuff")
      //  nd.m_Color = ezColor::CadetBlue;
    }

    if (const ezColorAttribute* pAttr = pRtti->GetAttributeByType<ezColorAttribute>())
    {
      nd.m_Color = pAttr->GetColor();
    }

    pRtti->GetAllProperties(properties);

    for (auto prop : properties)
    {
      if (prop->GetCategory() == ezPropertyCategory::Constant)
      {
        const ezAbstractConstantProperty* pConstant = static_cast<const ezAbstractConstantProperty*>(prop);
        if (!pConstant->GetConstant().CanConvertTo<ezUInt16>())
          continue;

        const ezUInt16 uiPinIndex = pConstant->GetConstant().ConvertTo<ezUInt16>();

        ezVisualScriptPinDescriptor pd;
        pd.m_sName = prop->GetPropertyName();
        pd.m_sTooltip = ""; /// \todo Use ezTranslateTooltip
        pd.m_uiPinIndex = uiPinIndex;

        if (const ezVisualScriptInputPinAttribute* pAttr = prop->GetAttributeByType<ezVisualScriptInputPinAttribute>())
        {
          pd.m_PinType = (pAttr->GetCategory() == ezVisualScriptPinCategory::Execution) ? ezVisualScriptPinDescriptor::PinType::Execution : ezVisualScriptPinDescriptor::PinType::Data;
          pd.m_Color = ezColor::Violet; /// \todo Category for color
          pd.m_pDataType = prop->GetSpecificType(); /// \todo Category for type

          if (pd.m_PinType == ezVisualScriptPinDescriptor::Execution)
          {
            pd.m_Color = ezColor::LightSlateGrey;
          }

          nd.m_InputPins.PushBack(pd);
        }

        if (const ezVisualScriptOutputPinAttribute* pAttr = prop->GetAttributeByType<ezVisualScriptOutputPinAttribute>())
        {
          pd.m_PinType = (pAttr->GetCategory() == ezVisualScriptPinCategory::Execution) ? ezVisualScriptPinDescriptor::PinType::Execution : ezVisualScriptPinDescriptor::PinType::Data;
          pd.m_Color = ezColor::Violet;  /// \todo Category for color
          pd.m_pDataType = prop->GetSpecificType(); /// \todo Category for type

          if (pd.m_PinType == ezVisualScriptPinDescriptor::Execution)
          {
            pd.m_Color = ezColor::LightSlateGrey;
          }

          nd.m_OutputPins.PushBack(pd);
        }
      }
      else
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

  //for (const auto& pin : nd.m_InputPins)
  //{
  //  if (pin.m_PropertyDesc.m_sName.IsEmpty())
  //    continue;

  //  desc.m_Properties.PushBack(pin.m_PropertyDesc);
  //}

  //for (const auto& pin : nd.m_OutputPins)
  //{
  //  if (pin.m_PropertyDesc.m_sName.IsEmpty())
  //    continue;

  //  desc.m_Properties.PushBack(pin.m_PropertyDesc);
  //}

  return ezPhantomRttiManager::RegisterType(desc);
}
