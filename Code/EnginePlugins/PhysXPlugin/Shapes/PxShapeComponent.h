#pragma once

#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <PhysXPlugin/Components/PxComponent.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

struct ezMsgExtractGeometry;

class EZ_PHYSXPLUGIN_DLL ezPxShapeComponent : public ezPxComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezPxShapeComponent, ezPxComponent);

public:
  ezPxShapeComponent();
  ~ezPxShapeComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  // ************************************* PROPERTIES ***********************************
public:
  // physics material
  // collision flags ?
  // flags: scene query shape, simulate, trigger (?), vdb vis

  void SetSurfaceFile(const char* szFile);
  const char* GetSurfaceFile() const;

  ezUInt32 GetShapeId() const { return m_uiShapeId; }

  ezUInt8 m_uiCollisionLayer = 0;
  bool m_bReportContact = false;

  float m_fContactImpactThreshold = 10.0f;

  ezHashedString m_sContactSurfaceInteraction;

protected:
  const char* GetContactSurfaceInteraction() const { return m_sContactSurfaceInteraction.GetData(); }
  void SetContactSurfaceInteraction(const char* sz) { m_sContactSurfaceInteraction.Assign(sz); }

  ezUInt32 m_uiShapeId = 0;

  ezSurfaceResourceHandle m_hSurface;

  ezPxUserData m_UserData;

  // ************************************* FUNCTIONS *****************************

public:
  void AddToActor(physx::PxRigidActor* pActor, const ezSimdTransform& parentTransform);
  virtual void ExtractGeometry(ezMsgExtractGeometry& msg) const;

protected:
  virtual physx::PxShape* CreateShape(physx::PxRigidActor* pActor, physx::PxTransform& out_ShapeTransform) = 0;

  physx::PxMaterial* GetPxMaterial();
  physx::PxFilterData CreateFilterData();
};
