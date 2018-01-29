#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <Core/World/Declarations.h>
#include <ProceduralPlacementPlugin/Tasks/PlacementTask.h>

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
    ezBoundingBox GetBoundingBox() const;
    ezColor GetDebugColor() const;

    void Update(ezPhysicsWorldModuleInterface* pPhysicsModule);

    bool IsFinished() const;

    ezUInt32 PlaceObjects(ezWorld& world);

  private:
    TileDesc m_Desc;
    ezSharedPtr<const Layer> m_pLayer;

    ezUniquePtr<PlacementTask> m_pPlacementTask;
    ezTaskGroupID m_TaskGroupId;

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
