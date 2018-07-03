#include <PCH.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementGraph.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementGraphQt.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementNodes.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Utilities/Node.h>

EZ_IMPLEMENT_SINGLETON(ezProceduralPlacementNodeRegistry);

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, ProceduralPlacement)

BEGIN_SUBSYSTEM_DEPENDENCIES
"PluginAssets", "ReflectedTypeManager"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  ezProceduralPlacementNodeRegistry* pRegistry = EZ_DEFAULT_NEW(ezProceduralPlacementNodeRegistry);

  pRegistry->UpdateNodeTypes();
  const ezRTTI* pBaseType = pRegistry->GetBaseType();

  //ezQtNodeScene::GetPinFactory().RegisterCreator(ezGetStaticRTTI<ezVisualScriptPin>(), [](const ezRTTI* pRtti)->ezQtPin* { return new ezQtVisualScriptPin(); });
  ezQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const ezRTTI* pRtti)->ezQtNode* { return new ezQtProceduralPlacementNode(); });
}

ON_CORE_SHUTDOWN
{
  ezProceduralPlacementNodeRegistry* pDummy = ezProceduralPlacementNodeRegistry::GetSingleton();
  EZ_DEFAULT_DELETE(pDummy);
}

ON_ENGINE_STARTUP
{
}

ON_ENGINE_SHUTDOWN
{
}

EZ_END_SUBSYSTEM_DECLARATION


ezProceduralPlacementNodeRegistry::ezProceduralPlacementNodeRegistry()
  : m_SingletonRegistrar(this)
{
  m_pBaseType = nullptr;
  m_pLayerOutputType = nullptr;
}

void ezProceduralPlacementNodeRegistry::UpdateNodeTypes()
{
  m_pBaseType = ezGetStaticRTTI<ezProceduralPlacementNodeBase>();
  m_pLayerOutputType = ezGetStaticRTTI<ezProceduralPlacementLayerOutput>();
}

//////////////////////////////////////////////////////////////////////////

bool ezProceduralPlacementNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(ezProceduralPlacementNodeRegistry::GetSingleton()->GetBaseType());
}

void ezProceduralPlacementNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node)
{
  const ezRTTI* pNodeBaseType = ezProceduralPlacementNodeRegistry::GetSingleton()->GetBaseType();

  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom(pNodeBaseType))
    return;

  ezHybridArray<ezAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    const ezRTTI* pPropType = pProp->GetSpecificType();
    if (!pPropType->IsDerivedFrom<ezNodePin>())
      continue;

    ezColor pinColor = ezColor::Grey;
    if (const ezColorAttribute* pAttr = pProp->GetAttributeByType<ezColorAttribute>())
    {
      pinColor = pAttr->GetColor();
    }

    if (pPropType->IsDerivedFrom<ezInputNodePin>())
    {
      ezPin* pPin = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Inputs.PushBack(pPin);
    }
    else if (pPropType->IsDerivedFrom<ezOutputNodePin>())
    {
      ezPin* pPin = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Outputs.PushBack(pPin);
    }
  }
}

void ezProceduralPlacementNodeManager::InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node)
{
  for (ezPin* pPin : node.m_Inputs)
  {
    EZ_DEFAULT_DELETE(pPin);
  }
  node.m_Inputs.Clear();
  for (ezPin* pPin : node.m_Outputs)
  {
    EZ_DEFAULT_DELETE(pPin);
  }
  node.m_Outputs.Clear();
}


void ezProceduralPlacementNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const
{
  const ezRTTI* pNodeBaseType = ezProceduralPlacementNodeRegistry::GetSingleton()->GetBaseType();

  for (auto it = ezRTTI::GetFirstInstance(); it != nullptr; it = it->GetNextInstance())
  {
    if (it->IsDerivedFrom(pNodeBaseType) && !it->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
      Types.PushBack(it);
  }
}

ezStatus ezProceduralPlacementNodeManager::InternalCanConnect(const ezPin* pSource, const ezPin* pTarget, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNto1;

  if (!pTarget->GetConnections().IsEmpty())
    return ezStatus("Only one connection can be made to in input pin!");

  return ezStatus(EZ_SUCCESS);
}

#if 0
const char* ezProceduralPlacementNodeManager::GetTypeCategory(const ezRTTI* pRtti) const
{
  const ezVisualScriptNodeDescriptor* pDesc = ezVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti);

  if (pDesc == nullptr)
    return nullptr;

  return pDesc->m_sCategory;
}
#endif

