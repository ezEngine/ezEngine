#pragma once

#include <GameUtils/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Reflection/Reflection.h>

typedef ezResourceHandle<class ezSurfaceResource> ezSurfaceResourceHandle;

struct EZ_GAMEUTILS_DLL ezSurfaceResourceDescriptor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSurfaceResourceDescriptor, ezReflectedClass);
public:

  void Load(ezStreamReader& stream);
  void Save(ezStreamWriter& stream) const;

  float m_fPhysicsRestitution;
  float m_fPhysicsFrictionStatic;
  float m_fPhysicsFrictionDynamic;
};

class EZ_GAMEUTILS_DLL ezSurfaceResource : public ezResource<ezSurfaceResource, ezSurfaceResourceDescriptor>
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

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezSurfaceResourceDescriptor& descriptor) override;

private:
  ezSurfaceResourceDescriptor m_Descriptor;
};

