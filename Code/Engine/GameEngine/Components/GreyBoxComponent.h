#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <RendererCore/Components/RenderComponent.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezMeshRenderData;
class ezGeometry;
struct ezExtractRenderDataMessage;
struct ezBuildStaticMeshMessage;
typedef ezTypedResourceHandle<class ezMeshResource> ezMeshResourceHandle;
typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;

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

    Default = Box
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezGreyBoxShape)

class EZ_GAMEENGINE_DLL ezGreyBoxComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezGreyBoxComponent, ezRenderComponent, ezGreyBoxComponentManager);

public:
  ezGreyBoxComponent();
  ~ezGreyBoxComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent Interface
public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;
  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // ezGreyBoxComponent Interface

  void SetShape(ezEnum<ezGreyBoxShape> shape);
  ezEnum<ezGreyBoxShape> GetShape() const { return m_Shape; }
  void SetMaterialFile(const char* szFile);
  const char* GetMaterialFile() const;
  void SetMaterial(const ezMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  ezMaterialResourceHandle GetMaterial() const { return m_hMaterial; }
  void SetSizeNegX(float f);
  float GetSizeNegX() const { return m_fSizeNegX; }
  void SetSizePosX(float f);
  float GetSizePosX() const { return m_fSizePosX; }
  void SetSizeNegY(float f);
  float GetSizeNegY() const { return m_fSizeNegY; }
  void SetSizePosY(float f);
  float GetSizePosY() const { return m_fSizePosY; }
  void SetSizeNegZ(float f);
  float GetSizeNegZ() const { return m_fSizeNegZ; }
  void SetSizePosZ(float f);
  float GetSizePosZ() const { return m_fSizePosZ; }
  void SetDetail(ezUInt32 uiDetail);
  ezUInt32 GetDetail() const { return m_uiDetail; }

  void OnBuildStaticMesh(ezBuildStaticMeshMessage& msg) const;

protected:
  ezEnum<ezGreyBoxShape> m_Shape;
  ezMaterialResourceHandle m_hMaterial;
  float m_fSizeNegX = 0;
  float m_fSizePosX = 0;
  float m_fSizeNegY = 0;
  float m_fSizePosY = 0;
  float m_fSizeNegZ = 0;
  float m_fSizePosZ = 0;
  ezUInt32 m_uiDetail = 8;

protected:
  void InvalidateMesh();
  void GenerateRenderMesh() const;
  void BuildGeometry(ezGeometry& geom) const;

  mutable ezMeshResourceHandle m_hMesh;
};

