#pragma once

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#    ifdef BUILDSYSTEM_BUILDING_PHYSXPLUGIN_LIB
#      define EZ_PHYSXPLUGIN_DLL __declspec(dllexport)
#    else
#      define EZ_PHYSXPLUGIN_DLL __declspec(dllimport)
#    endif
#  else
#    define EZ_PHYSXPLUGIN_DLL __attribute__((visibility("default")))
#  endif
#else
#  define EZ_PHYSXPLUGIN_DLL
#endif

namespace physx
{
  struct PxFilterData;
  class PxTransform;
  class PxGeometry;
  class PxShape;
  class PxConvexMesh;
  class PxTriangleMesh;
  class PxActor;
  class PxRigidActor;
  class PxRigidDynamic;
  class PxRigidStatic;
  class PxController;
  class PxCapsuleController;
  class PxMaterial;
  class PxJoint;
  class PxConstraint;
  class PxScene;
  class PxControllerManager;
  class PxQueryFilterCallback;
} // namespace physx
