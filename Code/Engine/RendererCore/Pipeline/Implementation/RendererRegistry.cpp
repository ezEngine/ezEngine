#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RendererRegistry.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RendererRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezRendererRegistry::UpdateRendererTypes();

    ezPlugin::Events().AddEventHandler(ezRendererRegistry::PluginEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezPlugin::Events().RemoveEventHandler(ezRendererRegistry::PluginEventHandler);

    ezRendererRegistry::ClearRendererInstances();
  }

EZ_END_SUBSYSTEM_DECLARATION;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderer, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezHybridArray<const ezRTTI*, 16> ezRendererRegistry::s_RendererTypes;
ezDynamicArray<ezUniquePtr<ezRenderer>> ezRendererRegistry::s_RendererInstances;
ezHashTable<const ezRTTI*, ezUInt32> ezRendererRegistry::s_RenderDataTypeToRendererIndex;
bool ezRendererRegistry::s_bRendererInstancesDirty = false;

// static
void ezRendererRegistry::PluginEventHandler(const ezPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case ezPluginEvent::AfterPluginChanges:
      UpdateRendererTypes();
      break;

    default:
      break;
  }
}

// static
void ezRendererRegistry::UpdateRendererTypes()
{
  s_RendererTypes.Clear();

  ezRTTI::ForEachDerivedType<ezRenderer>([](const ezRTTI* pRtti)
    { s_RendererTypes.PushBack(pRtti); },
    ezRTTI::ForEachOptions::ExcludeNonAllocatable);

  s_bRendererInstancesDirty = true;
}

// static
void ezRendererRegistry::CreateRendererInstances()
{
  if (!s_bRendererInstancesDirty)
    return;

  ClearRendererInstances();

  for (auto pRendererType : s_RendererTypes)
  {
    EZ_ASSERT_DEV(pRendererType->IsDerivedFrom(ezGetStaticRTTI<ezRenderer>()), "Renderer type '{}' must be derived from ezRenderer", pRendererType->GetTypeName());

    auto pRenderer = pRendererType->GetAllocator()->Allocate<ezRenderer>();

    ezUInt32 uiIndex = s_RendererInstances.GetCount();
    s_RendererInstances.PushBack(pRenderer);

    ezHybridArray<const ezRTTI*, 8> supportedTypes;
    pRenderer->GetSupportedRenderDataTypes(supportedTypes);

    for (auto pType : supportedTypes)
    {
      s_RenderDataTypeToRendererIndex.Insert(pType, uiIndex);
    }
  }

  s_bRendererInstancesDirty = false;
}

// static
void ezRendererRegistry::ClearRendererInstances()
{
  s_RendererInstances.Clear();
  s_RenderDataTypeToRendererIndex.Clear();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RendererRegistry);
