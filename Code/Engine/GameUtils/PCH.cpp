#include <GameUtils/PCH.h>

#include <Foundation/PCH.h>
#include <CoreUtils/PCH.h>
#include <Core/PCH.h>

EZ_STATICLINK_LIBRARY(GameUtils)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(GameUtils_DataStructures_Implementation_ObjectSelection);
  EZ_STATICLINK_REFERENCE(GameUtils_GridAlgorithms_Implementation_Rasterization);
  EZ_STATICLINK_REFERENCE(GameUtils_PathFinding_Implementation_GraphSearch);
  EZ_STATICLINK_REFERENCE(GameUtils_PathFinding_Implementation_GridNavmesh);
}



