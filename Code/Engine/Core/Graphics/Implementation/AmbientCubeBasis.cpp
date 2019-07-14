#include <CorePCH.h>

#include <Core/Graphics/AmbientCubeBasis.h>

ezVec3 ezAmbientCubeBasis::s_Dirs[NumDirs] = {
  ezVec3(1.0f, 0.0f, 0.0f),
  ezVec3(-1.0f, 0.0f, 0.0f),
  ezVec3(0.0f, 1.0f, 0.0f),
  ezVec3(0.0f, -1.0f, 0.0f),
  ezVec3(0.0f, 0.0f, 1.0f),
  ezVec3(0.0f, 0.0f, -1.0f)
};


EZ_STATICLINK_FILE(Core, Core_Graphics_Implementation_AmbientCubeBasis);

