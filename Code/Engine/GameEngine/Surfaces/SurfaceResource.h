#pragma once

#include <GameEngine/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/Surfaces/SurfaceResourceDescriptor.h>

class ezWorld;
class ezUuid;

class EZ_GAMEENGINE_DLL ezSurfaceResource : public ezResource<ezSurfaceResource, ezSurfaceResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSurfaceResource, ezResourceBase);

public:
  ezSurfaceResource();
  ~ezSurfaceResource();

  const ezSurfaceResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  struct Event
  {
    enum class Type
    {
      Created,
      Destroyed
    };

    Type m_Type;
    ezSurfaceResource* m_pSurface;
  };

  static ezEvent<const Event&, ezMutex> s_Events;

  void* m_pPhysicsMaterial;

  /// \brief Spawns the prefab that was defined for the given interaction at the given position and using the configured orientation.
  /// Returns false, if the interaction type was not defined in this surface or any of its base surfaces
  bool InteractWithSurface(ezWorld* pWorld, ezGameObjectHandle hObject, const ezVec3& vPosition, const ezVec3& vSurfaceNormal, const ezVec3& vIncomingDirection,
    const ezTempHashedString& sInteraction, const ezUInt16* pOverrideTeamID);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(ezSurfaceResourceDescriptor&& descriptor) override;

private:
  ezSurfaceResourceDescriptor m_Descriptor;

  ezHashTable<ezUInt32, const ezSurfaceInteraction*> m_Interactions;
};

