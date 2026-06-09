#include <EnginePluginTerrain/EnginePluginTerrainPCH.h>

EZ_STATICLINK_LIBRARY(EnginePluginTerrain)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(EnginePluginTerrain_SceneExport_TerrainHeightfieldExportModifier);
}
