#include <EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodeManager.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Utilities/Node.h>
#include <ToolsFoundation/Command/NodeCommands.h>

EZ_IMPLEMENT_SINGLETON(ezProcGenNodeRegistry);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, ProceduralPlacement)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "PluginAssets", "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezProcGenNodeRegistry* pRegistry = EZ_DEFAULT_NEW(ezProcGenNodeRegistry);

    pRegistry->UpdateNodeTypes();
    const ezRTTI* pBaseType = pRegistry->GetBaseType();

    ezQtNodeScene::GetPinFactory().RegisterCreator(ezGetStaticRTTI<ezPin>(), [](const ezRTTI* pRtti)->ezQtPin* { return new ezQtProcGenPin(); });
    ezQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const ezRTTI* pRtti)->ezQtNode* { return new ezQtProcGenNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezProcGenNodeRegistry* pDummy = ezProcGenNodeRegistry::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


ezProcGenNodeRegistry::ezProcGenNodeRegistry()
    : m_SingletonRegistrar(this)
{
  m_pBaseType = nullptr;
  m_pLayerOutputType = nullptr;
}

void ezProcGenNodeRegistry::UpdateNodeTypes()
{
  m_pBaseType = ezGetStaticRTTI<ezProcGenNodeBase>();
  m_pLayerOutputType = ezGetStaticRTTI<ezProcPlacementOutput>();
}

//////////////////////////////////////////////////////////////////////////

bool ezProcGenNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(ezProcGenNodeRegistry::GetSingleton()->GetBaseType());
}

void ezProcGenNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node)
{
  const ezRTTI* pNodeBaseType = ezProcGenNodeRegistry::GetSingleton()->GetBaseType();

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

void ezProcGenNodeManager::InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node)
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


void ezProcGenNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const
{
  const ezRTTI* pNodeBaseType = ezProcGenNodeRegistry::GetSingleton()->GetBaseType();

  for (auto it = ezRTTI::GetFirstInstance(); it != nullptr; it = it->GetNextInstance())
  {
    if (it->IsDerivedFrom(pNodeBaseType) && !it->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
      Types.PushBack(it);
  }
}

ezStatus ezProcGenNodeManager::InternalCanConnect(const ezPin* pSource, const ezPin* pTarget, CanConnectResult& out_Result) const
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
