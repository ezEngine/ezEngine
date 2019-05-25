#include <UtilitiesPCH.h>

EZ_STATICLINK_LIBRARY(Utilities)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(Utilities_DGML_Implementation_DGMLCreator);
  EZ_STATICLINK_REFERENCE(Utilities_DataStructures_Implementation_DynamicOctree);
  EZ_STATICLINK_REFERENCE(Utilities_DataStructures_Implementation_DynamicQuadtree);
  EZ_STATICLINK_REFERENCE(Utilities_DataStructures_Implementation_ObjectSelection);
  EZ_STATICLINK_REFERENCE(Utilities_FileFormats_Implementation_OBJLoader);
  EZ_STATICLINK_REFERENCE(Utilities_GridAlgorithms_Implementation_Rasterization);
  EZ_STATICLINK_REFERENCE(Utilities_PathFinding_Implementation_GridNavmesh);
}

