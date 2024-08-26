#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodeManager.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>
#include <Foundation/Configuration/Startup.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenPin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginProcGen, ProcGen)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    const ezRTTI* pBaseType = ezGetStaticRTTI<ezProcGenNodeBase>();

    ezQtNodeScene::GetPinFactory().RegisterCreator(ezGetStaticRTTI<ezProcGenPin>(), [](const ezRTTI* pRtti)->ezQtPin* { return new ezQtProcGenPin(); });
    ezQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const ezRTTI* pRtti)->ezQtNode* { return new ezQtProcGenNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const ezRTTI* pBaseType = ezGetStaticRTTI<ezProcGenNodeBase>();

    ezQtNodeScene::GetPinFactory().UnregisterCreator(ezGetStaticRTTI<ezProcGenPin>());
    ezQtNodeScene::GetNodeFactory().UnregisterCreator(pBaseType);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

bool ezProcGenNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(ezGetStaticRTTI<ezProcGenNodeBase>());
}

void ezProcGenNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node)
{
  const ezRTTI* pNodeBaseType = ezGetStaticRTTI<ezProcGenNodeBase>();

  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom(pNodeBaseType))
    return;

  ezHybridArray<const ezAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    const ezRTTI* pPropType = pProp->GetSpecificType();
    if (!pPropType->IsDerivedFrom<ezRenderPipelineNodePin>())
      continue;

    ezColor pinColor = ezColorScheme::DarkUI(ezColorScheme::Gray);
    if (const ezColorAttribute* pAttr = pProp->GetAttributeByType<ezColorAttribute>())
    {
      pinColor = pAttr->GetColor();
    }

    if (pPropType->IsDerivedFrom<ezRenderPipelineNodeInputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezProcGenPin, ezPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pPropType->IsDerivedFrom<ezRenderPipelineNodeOutputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezProcGenPin, ezPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Outputs.PushBack(pPin);
    }
  }
}

void ezProcGenNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const
{
  ezRTTI::ForEachDerivedType<ezProcGenNodeBase>(
    [&](const ezRTTI* pRtti)
    { ref_types.PushBack(pRtti); },
    ezRTTI::ForEachOptions::ExcludeAbstract);
}

ezStatus ezProcGenNodeManager::InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_result) const
{
  out_result = CanConnectResult::ConnectNto1;
  return ezStatus(EZ_SUCCESS);
}
