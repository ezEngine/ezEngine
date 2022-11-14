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
struct ezMsgTransformChanged;
using ezMeshResourceHandle = ezTypedResourceHandle<class ezMeshResource>;
using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;

typedef ezComponentManager<class ezGreyBoxComponent, ezBlockStorageType::Compact> ezGreyBoxComponentManager;

struct EZ_GAMEENGINE_DLL ezGreyBoxShape
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Box,
    RampX,
    RampY,
    Column,
    StairsX,
    StairsY,
    ArchX,
    ArchY,
    SpiralStairs,

    Default = Box
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezGreyBoxShape)

class EZ_GAMEENGINE_DLL ezGreyBoxComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezGreyBoxComponent, ezRenderComponent, ezGreyBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

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

  void SetShape(ezEnum<ezGreyBoxShape> shape);                // [ property ]
  ezEnum<ezGreyBoxShape> GetShape() const { return m_Shape; } // [ property ]
  void SetMaterialFile(const char* szFile);                   // [ property ]
  const char* GetMaterialFile() const;                        // [ property ]
  void SetSizeNegX(float f);                                  // [ property ]
  float GetSizeNegX() const { return m_fSizeNegX; }           // [ property ]
  void SetSizePosX(float f);                                  // [ property ]
  float GetSizePosX() const { return m_fSizePosX; }           // [ property ]
  void SetSizeNegY(float f);                                  // [ property ]
  float GetSizeNegY() const { return m_fSizeNegY; }           // [ property ]
  void SetSizePosY(float f);                                  // [ property ]
  float GetSizePosY() const { return m_fSizePosY; }           // [ property ]
  void SetSizeNegZ(float f);                                  // [ property ]
  float GetSizeNegZ() const { return m_fSizeNegZ; }           // [ property ]
  void SetSizePosZ(float f);                                  // [ property ]
  float GetSizePosZ() const { return m_fSizePosZ; }           // [ property ]
  void SetDetail(ezUInt32 uiDetail);                          // [ property ]
  ezUInt32 GetDetail() const { return m_uiDetail; }           // [ property ]
  void SetCurvature(ezAngle curvature);                       // [ property ]
  ezAngle GetCurvature() const { return m_Curvature; }        // [ property ]
  void SetSlopedTop(bool b);                                  // [ property ]
  bool GetSlopedTop() const { return m_bSlopedTop; }          // [ property ]
  void SetSlopedBottom(bool b);                               // [ property ]
  bool GetSlopedBottom() const { return m_bSlopedBottom; }    // [ property ]
  void SetThickness(float f);                                 // [ property ]
  float GetThickness() const { return m_fThickness; }         // [ property ]

  void SetGenerateCollision(bool b);                                 // [ property ]
  bool GetGenerateCollision() const { return m_bGenerateCollision; } // [ property ]

  void SetIncludeInNavmesh(bool b);                                // [ property ]
  bool GetIncludeInNavmesh() const { return m_bIncludeInNavmesh; } // [ property ]

  void SetMaterial(const ezMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  ezMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

protected:
  void OnBuildStaticMesh(ezMsgBuildStaticMesh& msg) const;
  void OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const;
  void OnMsgExtractOccluderData(ezMsgExtractOccluderData& msg) const;
  void OnMsgTransformChanged(ezMsgTransformChanged& msg);

  ezEnum<ezGreyBoxShape> m_Shape;
  ezMaterialResourceHandle m_hMaterial;
  ezColor m_Color = ezColor::White;
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
  bool m_bIncludeInNavmesh = true;
  bool m_bUseAsOccluder = true;

  void InvalidateMesh();
  void BuildGeometry(ezGeometry& geom, ezEnum<ezGreyBoxShape> shape, const ezMat4& transform, bool bOnlyRoughDetails) const;

  template <typename ResourceType>
  ezTypedResourceHandle<ResourceType> GenerateMesh() const;

  ezMeshResourceHandle m_hMesh;

  mutable ezSharedPtr<ezRasterizerObject> m_pOccluderObject;
};
