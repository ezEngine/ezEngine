#include <EditorPluginTerrain/EditorPluginTerrainPCH.h>

#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <EditorPluginTerrain/Visualizers/TerrainBrush2DVisualizerAdapter.h>
#include <EditorPluginTerrain/Visualizers/TerrainBrush3DVisualizerAdapter.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <TerrainPlugin/Components/TerrainBrushAttributes.h>

static void ezTerrainPatchComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  static const ezRTTI* pRtti = ezRTTI::FindTypeByName("ezTerrainPatchComponent");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const ezString sHeightImage = e.m_pObject->GetTypeAccessor().GetValue("HeightImage").ConvertTo<ezString>();
  const bool bHasHeightImage = !sHeightImage.IsEmpty();

  auto& props = *e.m_pPropertyStates;
  props["HeightImageOffset"].m_Visibility = bHasHeightImage ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  props["HeightImageSize"].m_Visibility = bHasHeightImage ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  props["HeightImageScale"].m_Visibility = bHasHeightImage ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
}

EZ_PLUGIN_ON_LOADED()
{
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezTerrainBrush2DVisualizerAttribute>(), [](const ezRTTI* pRtti) -> ezVisualizerAdapter*
    { return EZ_DEFAULT_NEW(ezTerrainBrush2DVisualizerAdapter); });
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezTerrainBrush3DVisualizerAttribute>(), [](const ezRTTI* pRtti) -> ezVisualizerAdapter*
    { return EZ_DEFAULT_NEW(ezTerrainBrush3DVisualizerAdapter); });
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezTerrainPatchComponent_PropertyMetaStateEventHandler);
}

EZ_PLUGIN_ON_UNLOADED()
{
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezTerrainBrush2DVisualizerAttribute>());
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezTerrainBrush3DVisualizerAttribute>());
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezTerrainPatchComponent_PropertyMetaStateEventHandler);
}
