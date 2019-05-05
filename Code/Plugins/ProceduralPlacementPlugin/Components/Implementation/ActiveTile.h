#pragma once

#include <ProceduralPlacementPlugin/ProceduralPlacementPluginDLL.h>
#include <Foundation/Types/UniquePtr.h>
#include <Core/World/Declarations.h>

class ezPhysicsWorldModuleInterface;

namespace ezPPInternal
{
  class ActiveTile
  {
  public:
    ActiveTile();
    ActiveTile(ActiveTile&& other);
    ~ActiveTile();

    void Initialize(const TileDesc& desc, ezSharedPtr<const Layer>& pLayer);
    void Deinitialize(ezWorld& world);

    bool IsValid() const;

    const TileDesc& GetDesc() const;
    const Layer* GetLayer() const;
    ezArrayPtr<const ezGameObjectHandle> GetPlacedObjects() const;
    ezBoundingBox GetBoundingBox() const;
    ezColor GetDebugColor() const;

    void PrepareTask(const ezPhysicsWorldModuleInterface* pPhysicsModule, PlacementTask& placementTask);

    ezUInt32 PlaceObjects(ezWorld& world, ezArrayPtr<const PlacementTransform> objectTransforms);

  private:
    TileDesc m_Desc;
    ezSharedPtr<const Layer> m_pLayer;

    struct State
    {
      enum Enum
      {
        Invalid,
        Initialized,
        Scheduled,
        Finished
      };
    };

    State::Enum m_State;
    ezDynamicArray<ezGameObjectHandle> m_PlacedObjects;
  };
}
