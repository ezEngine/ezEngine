#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Math/Vec2.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

class ezGeometry;
struct ezMsgExtractRenderData;
struct ezMsgBuildStaticMesh;
struct ezMsgExtractGeometry;
class ezHeightfieldComponent;
class ezMeshResourceDescriptor;

using ezMeshResourceHandle = ezTypedResourceHandle<class ezMeshResource>;
using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;
using ezImageDataResourceHandle = ezTypedResourceHandle<class ezImageDataResource>;

class EZ_GAMECOMPONENTS_DLL ezHeightfieldComponentManager : public ezComponentManager<ezHeightfieldComponent, ezBlockStorageType::Compact>
{
public:
  ezHeightfieldComponentManager(ezWorld* pWorld);
  ~ezHeightfieldComponentManager();

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);
  void AddToUpdateList(ezHeightfieldComponent* pComponent);

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  ezDeque<ezComponentHandle> m_ComponentsToUpdate;
};

/// \brief This component utilizes a greyscale image to generate an elevation mesh, which is typically used for simple terrain
///
/// The component always creates a mesh for rendering, which uses a single material.
/// For different layers of grass, dirt, etc. the material can combine multiple textures and a mask.
///
/// If the "GenerateCollision" property is set, the component also generates a static collision mesh during scene export.
class EZ_GAMECOMPONENTS_DLL ezHeightfieldComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezHeightfieldComponent, ezRenderComponent, ezHeightfieldComponentManager);

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
  // ezHeightfieldComponent

public:
  ezHeightfieldComponent();
  ~ezHeightfieldComponent();

  ezVec2 GetHalfExtents() const { return m_vHalfExtents; }       // [ property ]
  void SetHalfExtents(ezVec2 value);                             // [ property ]

  float GetHeight() const { return m_fHeight; }                  // [ property ]
  void SetHeight(float value);                                   // [ property ]

  ezVec2 GetTexCoordOffset() const { return m_vTexCoordOffset; } // [ property ]
  void SetTexCoordOffset(ezVec2 value);                          // [ property ]

  ezVec2 GetTexCoordScale() const { return m_vTexCoordScale; }   // [ property ]
  void SetTexCoordScale(ezVec2 value);                           // [ property ]

  void SetMaterial(const ezMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  ezMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

  void SetHeightfield(const ezImageDataResourceHandle& hResource);                   // [ property ]
  const ezImageDataResourceHandle& GetHeightfield() const { return m_hHeightfield; } // [ property ]

  ezVec2U32 GetTesselation() const { return m_vTesselation; }                        // [ property ]
  void SetTesselation(ezVec2U32 value);                                              // [ property ]

  void SetGenerateCollision(bool b);                                                 // [ property ]
  bool GetGenerateCollision() const { return m_bGenerateCollision; }                 // [ property ]

  ezVec2U32 GetColMeshTesselation() const { return m_vColMeshTesselation; }          // [ property ]
  void SetColMeshTesselation(ezVec2U32 value);                                       // [ property ]

protected:
  void OnBuildStaticMesh(ezMsgBuildStaticMesh& msg) const;                           // [ msg handler ]
  void OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const;                        // [ msg handler ]

  void InvalidateMesh();
  void BuildGeometry(ezGeometry& geom) const;
  ezResult BuildMeshDescriptor(ezMeshResourceDescriptor& desc) const;

  template <typename ResourceType>
  ezTypedResourceHandle<ResourceType> GenerateMesh() const;

  ezUInt32 m_uiHeightfieldChangeCounter = 0;
  ezImageDataResourceHandle m_hHeightfield;
  ezMaterialResourceHandle m_hMaterial;

  ezVec2 m_vHalfExtents = ezVec2(100.0f);
  float m_fHeight = 50.0f;

  ezVec2 m_vTexCoordOffset = ezVec2::MakeZero();
  ezVec2 m_vTexCoordScale = ezVec2(1);

  ezVec2U32 m_vTesselation = ezVec2U32(128);
  ezVec2U32 m_vColMeshTesselation = ezVec2U32(64);

  bool m_bGenerateCollision = true;

  ezMeshResourceHandle m_hMesh;
};
