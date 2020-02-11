#pragma once

#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <PhysXPlugin/Components/PxComponent.h>
#include <PhysXPlugin/PhysXInterface.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

struct ezMsgExtractGeometry;

class EZ_PHYSXPLUGIN_DLL ezPxShapeComponent : public ezPxComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezPxShapeComponent, ezPxComponent);


  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void Initialize() override;
  virtual void Deinitialize() override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxShapeComponent

public:
  ezPxShapeComponent();
  ~ezPxShapeComponent();

  virtual void ExtractGeometry(ezMsgExtractGeometry& msg) const;

  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  ezUInt32 GetShapeId() const { return m_uiShapeId; } // [ scriptable ]

  ezUInt8 m_uiCollisionLayer = 0;           // [ property ]
  ezBitflags<ezOnPhysXContact> m_OnContact; // [ property ]

protected:
  virtual physx::PxShape* CreateShape(physx::PxRigidActor* pActor, physx::PxTransform& out_ShapeTransform) = 0;

  ezUInt32 m_uiShapeId = ezInvalidIndex;

  ezSurfaceResourceHandle m_hSurface;

  ezPxUserData m_UserData;

  friend class ezPxActorComponent;
  void AddToActor(physx::PxRigidActor* pActor, const ezSimdTransform& parentTransform);

  physx::PxMaterial* GetPxMaterial();
  physx::PxFilterData CreateFilterData();
};
