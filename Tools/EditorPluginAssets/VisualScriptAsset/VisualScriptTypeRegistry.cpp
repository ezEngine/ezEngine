#include <PCH.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetManager.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraphQt.moc.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

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

void ezVisualScriptTypeRegistry::UpdateNodeData()
{
  /*ezVisualScriptNodeDescriptor nd;
  nd.m_sName = pNode->GetName();

  ExtractNodeConfig(pNode, nd);
  ExtractNodeProperties(pNode, nd);
  ExtractNodePins(pNode, "InputPin", nd.m_InputPins, false);
  ExtractNodePins(pNode, "OutputPin", nd.m_OutputPins, true);

  m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);*/

  // dummy
  {
    ezVisualScriptNodeDescriptor nd;
    nd.m_sName = "Dummy";
    nd.m_Color = ezColor::CadetBlue;
    nd.m_sCategory = "Test";

    {
      ezVisualScriptPinDescriptor pd;
      pd.m_Color = ezColor::Violet;
      pd.m_sName = "In 1";
      pd.m_sTooltip = "Tooltip";

      nd.m_InputPins.PushBack(pd);
    }

    {
      ezVisualScriptPinDescriptor pd;
      pd.m_Color = ezColor::Wheat;
      pd.m_sName = "Out 1";
      pd.m_sTooltip = "Tooltip";

      nd.m_OutputPins.PushBack(pd);
    }

    m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);
  }
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

const ezRTTI* ezVisualScriptTypeRegistry::GenerateTypeFromDesc(const ezVisualScriptNodeDescriptor& nd)
{
  ezStringBuilder temp;
  temp.Set("VisualScriptNode::", nd.m_sName);

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
