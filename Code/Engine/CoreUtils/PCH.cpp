#include <CoreUtils/PCH.h>
#include <Foundation/PCH.h>

EZ_STATICLINK_LIBRARY(CoreUtils)
{
  if(bReturn)
    return;

  EZ_STATICLINK_REFERENCE(CoreUtils_DataStructures_DynamicTree_Implementation_DynamicOctree);
  EZ_STATICLINK_REFERENCE(CoreUtils_DataStructures_DynamicTree_Implementation_DynamicQuadtree);
  EZ_STATICLINK_REFERENCE(CoreUtils_Graphics_Implementation_Camera);
  EZ_STATICLINK_REFERENCE(CoreUtils_ImageWriters_Implementation_BMPWriter);
  EZ_STATICLINK_REFERENCE(CoreUtils_Scripting_LuaWrapper_CFunctions);
  EZ_STATICLINK_REFERENCE(CoreUtils_Scripting_LuaWrapper_Initialize);
  EZ_STATICLINK_REFERENCE(CoreUtils_Scripting_LuaWrapper_Tables);
  EZ_STATICLINK_REFERENCE(CoreUtils_Scripting_LuaWrapper_Variables);
}



