#pragma once

#include <Core/Messages/EventMessage.h>
#include <PhysXPlugin/PhysXPluginDLL.h>
#include <PhysXPlugin/Utilities/PxUserData.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>

using ezBreakableSheetComponentManager = ezComponentManagerSimple<class ezBreakableSheetComponent, ezComponentUpdateType::Always>;

struct ezMsgExtractRenderData;
struct ezMsgCollision;
struct ezMsgPhysicsAddImpulse;
struct ezMsgExtractGeometry;
class ezShaderTransform;

/// \brief Sent when a breakable sheet breaks
struct EZ_PHYSXPLUGIN_DLL ezMsgBreakableSheetBroke : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgBreakableSheetBroke, ezEventMessage);

  /// The object that broke the sheet.
  ezGameObjectHandle m_hInstigatorObject;
};

/// \brief A breakable sheet is a 2D rectangular sheet (with a specified surface) which is breakable
class EZ_PHYSXPLUGIN_DLL ezBreakableSheetComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezBreakableSheetComponent, ezRenderComponent, ezBreakableSheetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // ezBreakableSheetComponent

public:
  ezBreakableSheetComponent();
  ~ezBreakableSheetComponent();

  ezBreakableSheetComponent& operator=(ezBreakableSheetComponent&& other);

  void SetWidth(float fWidth);                               // [ property ]
  float GetWidth() const;                                    // [ property ]

  void SetHeight(float fHeight);                             // [ property ]
  float GetHeight() const;                                   // [ property ]

  void SetThickness(float fThickness);                       // [ property ]
  float GetThickness() const;                                // [ property ]

  void SetDensity(float fDensity);                           // [ property ]
  float GetDensity() const;                                  // [ property ]

  void SetBreakImpulseStrength(float fBreakImpulseStrength); // [ property ]
  float GetBreakImpulseStrength() const;                     // [ property ]

  void SetDisappearTimeout(ezTime disappearTimeout);         // [ property ]
  ezTime GetDisappearTimeout() const;                        // [ property ]

  void SetFixedBorder(bool bFixedBorder);                    // [ property ]
  bool GetFixedBorder() const;                               // [ property ]

  void SetFixedRandomSeed(ezUInt32 uiFixedRandomSeed);       // [ property ]
  ezUInt32 GetFixedRandomSeed() const;                       // [ property ]

  void SetNumPieces(ezUInt32 uiNumPieces);                   // [ property ]
  ezUInt32 GetNumPieces() const;                             // [ property ]

  void SetMaterialFile(const char* szFile);                  // [ property ]
  const char* GetMaterialFile() const;                       // [ property ]

  void SetBrokenMaterialFile(const char* szFile);            // [ property ]
  const char* GetBrokenMaterialFile() const;                 // [ property ]

  ezUInt8 m_uiCollisionLayerUnbroken = 0;                    // [ property ]
  ezUInt8 m_uiCollisionLayerBrokenPieces = 0;                // [ property ]
  bool m_bIncludeInNavmesh = true;                           // [ property ]

  ezMaterialResourceHandle GetMaterial() const;
  ezMaterialResourceHandle GetBrokenMaterial() const;

  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg);          // [ msg handler ]

  void Break();                                                   // [ scriptable ]
  bool IsBroken() const { return m_bBroken; }                     // [ scriptable ]

  void OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const; // [ msg handler ]

protected:
  void Update();

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void OnCollision(ezMsgCollision& msg);

  float m_fWidth = 1.0f;
  float m_fHeight = 1.0f;
  float m_fThickness = 0.1f;
  float m_fBreakImpulseStrength = 25.0f;
  float m_fDensity = 1500.0f;
  ezTime m_DisappearTimeout;
  ezUInt32 m_uiFixedRandomSeed = 0;
  ezUInt32 m_uiNumPieces = 32;
  bool m_bFixedBorder = false;

  ezMaterialResourceHandle m_hMaterial;
  ezMaterialResourceHandle m_hBrokenMaterial;

  ezEventMessageSender<ezMsgBreakableSheetBroke> m_BreakEventSender;

  // State
  ezUInt32 m_uiRandomSeedUsed = 0;
  bool m_bBroken = false;
  ezMeshResourceHandle m_hUnbrokenMesh;
  ezMeshResourceHandle m_hPiecesMesh;
  ezDynamicArray<ezBoundingBox> m_PieceBoundingBoxes;
  ezBoundingSphere m_BrokenPiecesBoundingSphere;
  ezUInt32 m_uiNumActiveBrokenPieceActors = 0;
  ezTime m_TimeUntilDisappear;

  ezVec3 m_vExtents;

  ezSkinningState m_SkinningState;

  void BreakNow(const ezMsgCollision* pMessage = nullptr);
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
  ezUInt32 m_uiUnbrokenUserDataIndex = ezInvalidIndex;

  ezDynamicArray<physx::PxRigidDynamic*> m_PieceActors;
  ezDynamicArray<ezUInt32> m_PieceShapeIds;
  ezDynamicArray<ezUInt32> m_PieceUserDataIndices;
  ezMap<ezUInt32, physx::PxRigidDynamic*> m_ShapeIDsToActors;
};
