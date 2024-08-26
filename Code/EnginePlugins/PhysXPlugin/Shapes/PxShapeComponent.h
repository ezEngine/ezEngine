#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
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
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxShapeComponent

public:
  ezPxShapeComponent();
  ~ezPxShapeComponent();

  virtual void ExtractGeometry(ezMsgExtractGeometry& ref_msg) const;

  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  /// \brief Sets the shape ID for the object to use. This can only be set right after creation, before the component is activated.
  void SetInitialShapeId(ezUInt32 uiId);
  ezUInt32 GetShapeId() const { return m_uiShapeId; } // [ scriptable ]

  ezUInt8 m_uiCollisionLayer = 0;                     // [ property ]
  ezBitflags<ezOnPhysXContact> m_OnContact;           // [ property ]
  ezSurfaceResourceHandle m_hSurface;

protected:
  virtual void CreateShapes(ezDynamicArray<physx::PxShape*>& out_Shapes, physx::PxRigidActor* pActor, physx::PxTransform& out_ShapeTransform) = 0;

  ezUInt32 m_uiShapeId = ezInvalidIndex;
  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;

  friend class ezPxActorComponent;
  void AddToActor(physx::PxRigidActor* pActor, const ezSimdTransform& parentTransform);

  physx::PxMaterial* GetPxMaterial();
  physx::PxFilterData CreateFilterData();
};
