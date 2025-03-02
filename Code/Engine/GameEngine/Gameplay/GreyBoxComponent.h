#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>

class ezMeshRenderData;
class ezGeometry;
struct ezMsgExtractRenderData;
struct ezMsgBuildStaticMesh;
struct ezMsgExtractGeometry;
struct ezMsgExtractOccluderData;
struct ezMsgSetMeshMaterial;
struct ezMsgSetColor;
class ezMeshResourceDescriptor;
struct ezMsgSetCustomData;
using ezMeshResourceHandle = ezTypedResourceHandle<class ezMeshResource>;
using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;

using ezGreyBoxComponentManager = ezComponentManager<class ezGreyBoxComponent, ezBlockStorageType::Compact>;

struct EZ_GAMEENGINE_DLL ezGreyBoxShape
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Box,
    RampPosX,
    RampNegX,
    RampPosY,
    RampNegY,
    Column,
    StairsPosX,
    StairsNegX,
    StairsPosY,
    StairsNegY,
    ArchX,
    ArchY,
    SpiralStairs,

    Default = Box
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezGreyBoxShape)

/// \brief Creates basic geometry for prototyping levels.
///
/// It automatically creates physics collision geometry and also sets up rendering occluders to improve performance.
class EZ_GAMEENGINE_DLL ezGreyBoxComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezGreyBoxComponent, ezRenderComponent, ezGreyBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent
protected:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // ezGreyBoxComponent

public:
  ezGreyBoxComponent();
  ~ezGreyBoxComponent();

  /// \brief The geometry type to build.
  void SetShape(ezEnum<ezGreyBoxShape> shape);                // [ property ]
  ezEnum<ezGreyBoxShape> GetShape() const { return m_Shape; } // [ property ]

  /// \brief An additional tint color passed to the renderer to modify the mesh.
  void SetColor(const ezColor& color); // [ property ]
  const ezColor& GetColor() const;     // [ property ]

  /// \brief An additional vec4 passed to the renderer that can be used by custom material shaders for effects.
  void SetCustomData(const ezVec4& vData); // [ property ]
  const ezVec4& GetCustomData() const;     // [ property ]

  /// \brief Sets the extent along the negative X axis of the bounding box.
  void SetSizeNegX(float f);                        // [ property ]
  float GetSizeNegX() const { return m_fSizeNegX; } // [ property ]

  /// \brief Sets the extent along the positive X axis of the bounding box.
  void SetSizePosX(float f);                        // [ property ]
  float GetSizePosX() const { return m_fSizePosX; } // [ property ]

  /// \brief Sets the extent along the negative Y axis of the bounding box.
  void SetSizeNegY(float f);                        // [ property ]
  float GetSizeNegY() const { return m_fSizeNegY; } // [ property ]

  /// \brief Sets the extent along the positive Y axis of the bounding box.
  void SetSizePosY(float f);                        // [ property ]
  float GetSizePosY() const { return m_fSizePosY; } // [ property ]

  /// \brief Sets the extent along the negative Z axis of the bounding box.
  void SetSizeNegZ(float f);                        // [ property ]
  float GetSizeNegZ() const { return m_fSizeNegZ; } // [ property ]

  /// \brief Sets the extent along the positive Z axis of the bounding box.
  void SetSizePosZ(float f);                        // [ property ]
  float GetSizePosZ() const { return m_fSizePosZ; } // [ property ]

  /// \brief Sets the detail of the geometry. The meaning is geometry type specific, e.g. for cylinders this is the number of polygons around the perimeter.
  void SetDetail(ezUInt32 uiDetail);                // [ property ]
  ezUInt32 GetDetail() const { return m_uiDetail; } // [ property ]

  /// \brief Geometry type specific: Sets an angle, used to curve stairs, etc.
  void SetCurvature(ezAngle curvature);                // [ property ]
  ezAngle GetCurvature() const { return m_Curvature; } // [ property ]

  /// \brief For curved stairs to make the top smooth.
  void SetSlopedTop(bool b);                         // [ property ]
  bool GetSlopedTop() const { return m_bSlopedTop; } // [ property ]

  /// \brief For curved stairs to make the bottom smooth.
  void SetSlopedBottom(bool b);                            // [ property ]
  bool GetSlopedBottom() const { return m_bSlopedBottom; } // [ property ]

  /// \brief Geometry type specific: Sets a thickness, e.g. for curved stairs.
  void SetThickness(float f);                         // [ property ]
  float GetThickness() const { return m_fThickness; } // [ property ]

  /// \brief Whether the mesh should be used as a collider.
  void SetGenerateCollision(bool b);                                 // [ property ]
  bool GetGenerateCollision() const { return m_bGenerateCollision; } // [ property ]

  /// \brief Sets the ezMaterialResource to use for rendering.
  void SetMaterial(const ezMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  ezMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

protected:
  void OnBuildStaticMesh(ezMsgBuildStaticMesh& msg) const;
  void OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const;
  void OnMsgExtractOccluderData(ezMsgExtractOccluderData& msg) const;

  void OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& ref_msg); // [ msg handler ]
  void OnMsgSetColor(ezMsgSetColor& ref_msg);               // [ msg handler ]
  void OnMsgSetCustomData(ezMsgSetCustomData& ref_msg);     // [ msg handler ]

  ezEnum<ezGreyBoxShape> m_Shape;
  ezMaterialResourceHandle m_hMaterial;
  ezColor m_Color = ezColor::White;
  ezVec4 m_vCustomData = ezVec4(0, 1, 0, 1);
  float m_fSizeNegX = 0;
  float m_fSizePosX = 0;
  float m_fSizeNegY = 0;
  float m_fSizePosY = 0;
  float m_fSizeNegZ = 0;
  float m_fSizePosZ = 0;
  ezUInt32 m_uiDetail = 16;
  ezAngle m_Curvature;
  float m_fThickness = 0.5f;
  bool m_bSlopedTop = false;
  bool m_bSlopedBottom = false;
  bool m_bGenerateCollision = true;
  bool m_bUseAsOccluder = true;

  void InvalidateMesh();
  void BuildGeometry(ezGeometry& geom, ezEnum<ezGreyBoxShape> shape, bool bOnlyRoughDetails) const;

  template <typename ResourceType>
  ezTypedResourceHandle<ResourceType> GenerateMesh() const;

  void GenerateMeshName(ezStringBuilder& out_sName) const;
  void GenerateMeshResourceDescriptor(ezMeshResourceDescriptor& desc) const;

  ezMeshResourceHandle m_hMesh;

  mutable ezSharedPtr<const ezRasterizerObject> m_pOccluderObject;
};
