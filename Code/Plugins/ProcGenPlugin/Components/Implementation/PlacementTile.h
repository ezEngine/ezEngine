#pragma once

#include <ProcGenPlugin/Declarations.h>
#include <Foundation/Types/UniquePtr.h>
#include <Core/World/Declarations.h>

class ezPhysicsWorldModuleInterface;

namespace ezProcGenInternal
{
  class PlacementTile
  {
  public:
    PlacementTile();
    PlacementTile(PlacementTile&& other);
    ~PlacementTile();

    void Initialize(const PlacementTileDesc& desc, ezSharedPtr<const PlacementOutput>& pOutput);
    void Deinitialize(ezWorld& world);

    bool IsValid() const;

    const PlacementTileDesc& GetDesc() const;
    const PlacementOutput* GetOutput() const;
    ezArrayPtr<const ezGameObjectHandle> GetPlacedObjects() const;
    ezBoundingBox GetBoundingBox() const;
    ezColor GetDebugColor() const;

    void PrepareTask(const ezPhysicsWorldModuleInterface* pPhysicsModule, PlacementTask& placementTask);

    ezUInt32 PlaceObjects(ezWorld& world, ezArrayPtr<const PlacementTransform> objectTransforms);

  private:
    PlacementTileDesc m_Desc;
    ezSharedPtr<const PlacementOutput> m_pOutput;

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
