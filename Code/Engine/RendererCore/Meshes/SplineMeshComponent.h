#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

struct ezMsgSplineChanged;
struct ezMsgExtractGeometry;

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
  float m_fPaddingFront = 0.0f;
  float m_fPaddingBack = 0.0f;

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

/// \brief TODO: Documentation
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

  void SetStartPart(const ezSplineMeshPart& part);                                                // [ property ]
  const ezSplineMeshPart& GetStartPart() const { return m_StartPart; }                            // [ property ]

  void SetMiddleParts(ezArrayPtr<const ezSplineMeshPart> middleParts);                            // [ property ]
  ezArrayPtr<const ezSplineMeshPart> GetMiddleParts() const { return m_MiddleParts; }             // [ property ]

  void SetEndPart(const ezSplineMeshPart& part);                                                  // [ property ]
  const ezSplineMeshPart& GetEndPart() const { return m_EndPart; }                                // [ property ]

  void SetDistributionMode(ezEnum<ezSplineMeshDistributionMode> mode);                            // [ property ]
  ezEnum<ezSplineMeshDistributionMode> GetDistributionMode() const { return m_DistributionMode; } // [ property ]

  void SetSeed(ezInt32 iSeed);                                                                    // [ property ]
  ezInt32 GetSeed() const { return m_iSeed; }                                                     // [ property ]

  void SetOffsetY(float fOffsetY);                                                                // [ property ]
  float GetOffsetY() const { return m_fOffsetY; }                                                 // [ property ]

  void SetOffsetZ(float fOffsetZ);                                                                // [ property ]
  float GetOffsetZ() const { return m_fOffsetZ; }                                                 // [ property ]

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

  void GenerateMeshPath(const ezSplineComponent& splineComponent, ezStringBuilder& out_sSplineMeshPath) const;

  ezResult GenerateDistribution(const ezSplineComponent& splineComponent, ezDynamicArray<ezMeshResourceHandle>& out_Meshes, ezDynamicArray<ezVec2>& out_scaleOffsets) const;
  ezUInt32 FindBestMiddlePart(float fRequestedLength, ezArrayPtr<const ezVec2> middleLengthAndOffset, bool bAllowOverlapFront, bool bAllowOverlapBack, int& inout_iRandomPos) const;

  ezResult GenerateSplineMesh(const ezSplineComponent& splineComponent, ezMeshResourceDescriptor& out_splineMeshDesc, ezMsgGenerateSplineMeshCollision* out_pMsg = nullptr) const;
  void UpdateSplineMesh();

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
};
