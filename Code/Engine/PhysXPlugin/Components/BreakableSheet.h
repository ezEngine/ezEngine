#pragma once

#include <PhysXPlugin/Basics.h>
#include <PhysXPlugin/Utilities/PxUserData.h>
#include <Core/Messages/EventMessage.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

//typedef ezComponentManager<class ezBreakableSheetComponent, ezBlockStorageType::FreeList> ezBreakableSheetComponentManager;
typedef ezComponentManagerSimple<class ezBreakableSheetComponent, ezComponentUpdateType::Always /* TODO: When simulating */> ezBreakableSheetComponentManager;

struct ezExtractRenderDataMessage;
struct ezBuildNavMeshMessage;
struct ezCollisionMessage;
struct ezPhysicsAddImpulseMsg;

/// \brief Sent when a breakable sheet breaks
struct EZ_PHYSXPLUGIN_DLL ezBreakableSheetBreakEventMessage : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezBreakableSheetBreakEventMessage, ezEventMessage);

  /// The object that broke the sheet.
  ezGameObjectHandle m_hInstigatorObject;
};

/// \brief A breakable sheet is a 2D rectangular sheet (with a specified surface) which is breakable
class EZ_PHYSXPLUGIN_DLL ezBreakableSheetComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezBreakableSheetComponent, ezRenderComponent, ezBreakableSheetComponentManager);

public:
  ezBreakableSheetComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

  void OnBuildNavMesh(ezBuildNavMeshMessage& msg) const;
  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;
  void OnCollision(ezCollisionMessage& msg);
  void AddImpulseAtPos(ezPhysicsAddImpulseMsg& msg);

  bool IsBroken() const { return m_bBroken; }

protected:

  virtual void Initialize() override;
  virtual void OnSimulationStarted() override;
  virtual void Deinitialize() override;

public:

  // ************************************* PROPERTIES ***********************************

  void SetWidth(float fWidth);
  float GetWidth() const;

  void SetHeight(float fHeight);
  float GetHeight() const;

  void SetThickness(float fThickness);
  float GetThickness() const;

  void SetDensity(float fDensity);
  float GetDensity() const;

  void SetBreakImpulseStrength(float fBreakImpulseStrength);
  float GetBreakImpulseStrength() const;

  void SetDisappearTimeout(ezTime fDisappearTimeout);
  ezTime GetDisappearTimeout() const;

  void SetFixedBorder(bool bFixedBorder);
  bool GetFixedBorder() const;

  void SetFixedRandomSeed(ezUInt32 uiFixedRandomSeed);
  ezUInt32 GetFixedRandomSeed() const;

  void SetNumPieces(ezUInt32 uiNumPieces);
  ezUInt32 GetNumPieces() const;

  void SetMaterialFile(const char* szFile);
  const char* GetMaterialFile() const;

  ezMaterialResourceHandle GetMaterial() const;

  void SetBrokenMaterialFile(const char* szFile);
  const char* GetBrokenMaterialFile() const;

  ezMaterialResourceHandle GetBrokenMaterial() const;

  ezUInt8 m_uiCollisionLayerUnbroken = 0;
  ezUInt8 m_uiCollisionLayerBrokenPieces = 0;
  bool m_bIncludeInNavmesh = true;

protected:

  // Property variables
  float m_fWidth = 1.0f;
  float m_fHeight = 1.0f;
  float m_fThickness = 0.1f;
  float m_fBreakImpulseStrength = 25.0f;
  float m_fDensity = 1500.0f;
  ezTime m_fDisappearTimeout;
  ezUInt32 m_uiFixedRandomSeed = 0;
  ezUInt32 m_uiNumPieces = 32;
  bool m_bFixedBorder = false;

  ezMaterialResourceHandle m_hMaterial;
  ezMaterialResourceHandle m_hBrokenMaterial;

  ezEventMessageSender<ezBreakableSheetBreakEventMessage> m_BreakEventSender;

  // State
  ezUInt32 m_uiRandomSeedUsed = 0;
  bool m_bBroken = false;
  bool m_bPiecesMovedThisFrame = false;
  ezMeshResourceHandle m_hUnbrokenMesh;
  ezMeshResourceHandle m_hPiecesMesh;
  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper> m_PieceTransforms;
  ezDynamicArray<ezBoundingBox> m_PieceBoundingBoxes;
  ezBoundingSphere m_BrokenPiecesBoundingSphere;
  ezUInt32 m_uiNumActiveBrokenPieceActors = 0;
  float m_fTimeUntilDisappear = 0.0f;

  ezVec3 m_vExtents;

  ezGALBufferHandle m_hPieceTransformsBuffer;

  // ************************************* FUNCTIONS *****************************
  void Break(const ezCollisionMessage* pMessage = nullptr);
  void CreateMeshes();
  void AddSkirtPolygons(ezVec2 Point0, ezVec2 Point1, float fHalfThickness, ezInt32 iPieceMatrixIndex, ezGeometry& Geometry) const;
  void BuildMeshResourceFromGeometry(ezGeometry& Geometry, ezMeshResourceDescriptor& MeshDesc, bool bWithSkinningData) const;
  void UpdateBrokenPiecesBoundingSphere();

  void CreateUnbrokenPhysicsObject();
  void DestroyUnbrokenPhysicsObject();

  void CreatePiecesPhysicsObjects(ezVec3 vImpulse, ezVec3 vPointOfBreakage);
  void DestroyPiecesPhysicsObjects();

  void ReinitMeshes();
  void Cleanup();

  friend class ezPxDynamicActorComponentManager;
  void SetPieceTransform(const physx::PxTransform& transform, void* pAdditionalUserData);

  // PhysX Objects
  physx::PxRigidStatic* m_pUnbrokenActor = nullptr;
  ezUInt32 m_uiUnbrokenShapeId = ezInvalidIndex;
  ezPxUserData m_UnbrokenUserData;

  ezDynamicArray<physx::PxRigidDynamic*> m_PieceActors;
  ezDynamicArray<ezUInt32> m_PieceShapeIds;
  ezDynamicArray<ezPxUserData> m_PieceUserDatas;
  ezMap<ezUInt32, physx::PxRigidDynamic*> m_ShapeIDsToActors;
};
