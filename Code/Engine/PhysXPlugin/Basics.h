#pragma once

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_PHYSXPLUGIN_LIB
    #define EZ_PHYSXPLUGIN_DLL __declspec(dllexport)
  #else
    #define EZ_PHYSXPLUGIN_DLL __declspec(dllimport)
  #endif
#else
  #define EZ_PHYSXPLUGIN_DLL
#endif

namespace physx
{
  struct PxFilterData;
  struct PxActiveTransform;
  class PxTransform;
  class PxGeometry;
  class PxShape;
  class PxConvexMesh;
  class PxTriangleMesh;
  class PxRigidActor;
  class PxRigidDynamic;
  class PxRigidStatic;
  class PxCapsuleController;
  class PxMaterial;
  class PxJoint;
  class PxScene;
  class PxControllerManager;
}
