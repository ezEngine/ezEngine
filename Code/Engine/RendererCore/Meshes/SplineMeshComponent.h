#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

#include <Foundation/Containers/ArrayMap.h>

struct ezMsgSplineChanged;
struct ezMsgExtractGeometry;
struct ezMsgGenericEvent;
struct ezSpline;
class ezAbstractObjectNode;
class ezSplineComponent;

/// \brief Determines how many meshes are distributed along the spline and how they are scaled.
struct ezSplineMeshDistributionMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    FitToSegment,          ///< Each mesh is stretched to fit exactly into one spline segment.
    ScaleEvenly,           ///< Mesh length is used to determine the number of meshes distributed along the whole spline.
    ScaleEvenlyPerSegment, ///< Mesh length is used to determine the number of meshes distributed along each spline segment. This is useful when parts should align with spline nodes.

    Default = ScaleEvenly
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSplineMeshDistributionMode);

//////////////////////////////////////////////////////////////////////////

/// \brief Message sent by the ezSplineMeshComponent to request collision mesh generation.
struct EZ_RENDERERCORE_DLL ezMsgGenerateSplineMeshCollision : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgGenerateSplineMeshCollision, ezMessage);

  ezComponentHandle m_hSplineComponent;
  ezSmallArray<ezMeshResourceHandle, 4> m_RenderMeshes;
  ezSmallArray<ezVec2, 4> m_ScaleOffsets;
  float m_fLocalOffsetY = 0.0f;
  float m_fLocalOffsetZ = 0.0f;
};

//////////////////////////////////////////////////////////////////////////

struct EZ_RENDERERCORE_DLL ezSplineMeshPart
{
  ezMeshResourceHandle m_hMesh;
  float m_fPaddingFront = 0.0f; ///< Adds padding in front of this part. Can be negative to overlap parts.
  float m_fPaddingBack = 0.0f;  ///< Adds padding at the back of this part. Can be negative to overlap parts.

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  bool IsValid() const
  {
    return m_hMesh.IsValid();
  }
  ezResult ComputeLengthAndOffset(ezVec2& out_vLengthAndOffset) const;
  ezVec2 AddPadding(const ezVec2& vLengthAndOffset, bool bAllowOverlapFront, bool bAllowOverlapBack) const;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSplineMeshPart);

//////////////////////////////////////////////////////////////////////////

using ezSplineMeshComponentManager = ezComponentManager<class ezSplineMeshComponent, ezBlockStorageType::Compact>;

/// \brief A component that generates a mesh along a spline using the specified mesh parts.
///
/// The spline is taken from an ezSplineComponent on the owner game object or one of its parents.
/// Generation only happens in the editor and the generated mesh is cached to disk so it can be loaded at runtime.
class EZ_RENDERERCORE_DLL ezSplineMeshComponent : public ezMeshComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezSplineMeshComponent, ezMeshComponentBase, ezSplineMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSplineMeshComponent

public:
  ezSplineMeshComponent();
  ~ezSplineMeshComponent();

  /// \brief Sets a specialized mesh part to be used at the start of the spline. Can be empty.
  void SetStartPart(const ezSplineMeshPart& part);                     // [ property ]
  const ezSplineMeshPart& GetStartPart() const { return m_StartPart; } // [ property ]

  /// \brief Sets the list of mesh parts to be used in the middle of the spline. At least one part must be specified.
  ///
  /// If multiple parts are specified either a random one or the best fitting one is chosen,
  /// depending on the distribution mode and the spline and part length.
  void SetMiddleParts(ezArrayPtr<const ezSplineMeshPart> middleParts);                // [ property ]
  ezArrayPtr<const ezSplineMeshPart> GetMiddleParts() const { return m_MiddleParts; } // [ property ]

  /// \brief Sets a specialized mesh part to be used at the end of the spline. Can be empty.
  void SetEndPart(const ezSplineMeshPart& part);                   // [ property ]
  const ezSplineMeshPart& GetEndPart() const { return m_EndPart; } // [ property ]

  /// \brief Sets how the meshes are distributed along the spline.
  void SetDistributionMode(ezEnum<ezSplineMeshDistributionMode> mode);                            // [ property ]
  ezEnum<ezSplineMeshDistributionMode> GetDistributionMode() const { return m_DistributionMode; } // [ property ]

  /// \brief Sets the random seed used when selecting middle parts randomly.
  ///
  /// Negative values indicate to use the stable random seed from the owner object.
  /// Positive values or zero specify an explicit seed value.
  void SetSeed(ezInt32 iSeed);                // [ property ]
  ezInt32 GetSeed() const { return m_iSeed; } // [ property ]

  /// \brief Sets an offset that is applied to each generated mesh vertex in the local Y direction of the spline
  /// effectively moving the mesh to the left or right of the spline.
  void SetOffsetY(float fOffsetY);                // [ property ]
  float GetOffsetY() const { return m_fOffsetY; } // [ property ]

  /// \brief Sets an offset that is applied to each generated mesh vertex in the local Z direction of the spline
  /// effectively moving the mesh up or down relative to the spline.
  void SetOffsetZ(float fOffsetZ);                // [ property ]
  float GetOffsetZ() const { return m_fOffsetZ; } // [ property ]

  /// \brief Helper function to generate a spline mesh descriptor from the given spline and meshes.
  static ezResult GenerateSplineMeshDesc(const ezSpline& spline, const ezArrayMap<float, float>& distanceToKey, ezArrayPtr<ezCpuMeshResource*> meshes, ezArrayPtr<ezVec2> scaleOffsets, float fLocalOffsetY, float fLocalOffsetZ, ezMeshResourceDescriptor& out_splineMeshDesc);

private:
  ezUInt32 MiddleParts_GetCount() const { return m_MiddleParts.GetCount(); }
  const ezSplineMeshPart& MiddleParts_GetValue(ezUInt32 uiIndex) const { return m_MiddleParts[uiIndex]; }
  void MiddleParts_SetValue(ezUInt32 uiIndex, const ezSplineMeshPart& value);
  void MiddleParts_Insert(ezUInt32 uiIndex, const ezSplineMeshPart& value);
  void MiddleParts_Remove(ezUInt32 uiIndex);

  void OnObjectCreated(const ezAbstractObjectNode& node);
  void OnMsgSplineChanged(ezMsgSplineChanged& ref_msg);           // [ msg handler ]
  void OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const; // [ msg handler ]
  void OnMsgGenericEvent(ezMsgGenericEvent& ref_msg);             // [ msg handler ]

  void GenerateMeshPath(const ezSplineComponent& splineComponent, ezStringBuilder& out_sSplineMeshPath) const;

  ezResult GenerateDistribution(const ezSplineComponent& splineComponent, ezDynamicArray<ezMeshResourceHandle>& out_Meshes, ezDynamicArray<ezVec2>& out_scaleOffsets) const;
  ezUInt32 FindBestMiddlePart(float fRequestedLength, ezArrayPtr<const ezVec2> middleLengthAndOffset, bool bAllowOverlapFront, bool bAllowOverlapBack, int& inout_iRandomPos, ezUInt32 uiSeed) const;

  void UpdateSplineMesh();

  void StartGenerateTask(ezSharedPtr<ezTask>&& pTask);

  const ezSplineComponent* GetSplineComponent() const;

  ezSplineMeshPart m_StartPart;
  ezSmallArray<ezSplineMeshPart, 1> m_MiddleParts;
  ezSplineMeshPart m_EndPart;

  ezInt32 m_iSeed = -1;
  ezEnum<ezSplineMeshDistributionMode> m_DistributionMode;

  ezUInt16 m_uiLastSplineChangeCounter = 0;

  float m_fOffsetY = 0.0f;
  float m_fOffsetZ = 0.0f;

  ezUInt64 m_uiStableId = 0;

  ezSharedPtr<ezTask> m_pGenerationTask;
  ezSharedPtr<ezTask> m_pNextGenerationTask;
  ezTaskGroupID m_TaskGroupID;
};
