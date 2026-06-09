#include <EditorPluginTerrain/EditorPluginTerrainPCH.h>

#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <EditorPluginTerrain/Visualizers/TerrainBrush2DVisualizerAdapter.h>
#include <EditorPluginTerrain/Visualizers/TerrainBrush3DVisualizerAdapter.h>
#include <TerrainPlugin/Components/TerrainBrushAttributes.h>

EZ_PLUGIN_ON_LOADED()
{
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezTerrainBrush2DVisualizerAttribute>(), [](const ezRTTI* pRtti) -> ezVisualizerAdapter*
    { return EZ_DEFAULT_NEW(ezTerrainBrush2DVisualizerAdapter); });
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezTerrainBrush3DVisualizerAttribute>(), [](const ezRTTI* pRtti) -> ezVisualizerAdapter*
    { return EZ_DEFAULT_NEW(ezTerrainBrush3DVisualizerAdapter); });
}

EZ_PLUGIN_ON_UNLOADED()
{
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezTerrainBrush2DVisualizerAttribute>());
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezTerrainBrush3DVisualizerAttribute>());
}
