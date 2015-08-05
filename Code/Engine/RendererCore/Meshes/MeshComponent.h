#pragma once

#include <Core/World/World.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/Declarations.h>

class EZ_RENDERERCORE_DLL ezMeshRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshRenderData);

public:
  ezTransform m_GlobalTransform;
  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hMaterial;
  ezUInt32 m_uiPartIndex;
  ezUInt32 m_uiEditorPickingID;
  ezColor m_MeshColor;
};

class ezMeshComponent;
typedef ezComponentManager<ezMeshComponent> ezMeshComponentManager;

class EZ_RENDERERCORE_DLL ezMeshComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezMeshComponent, ezMeshComponentManager);

public:
  ezMeshComponent();

  void SetMesh(const ezMeshResourceHandle& hMesh);
  EZ_FORCE_INLINE const ezMeshResourceHandle& GetMesh() const
  {
    return m_hMesh;
  }

  EZ_FORCE_INLINE void SetMaterial(ezUInt32 uiIndex, const ezMaterialResourceHandle& hMaterial)
  {
    if (uiIndex >= m_Materials.GetCount())
      m_Materials.SetCount(uiIndex + 1);

    m_Materials[uiIndex] = hMaterial;
  }

  EZ_FORCE_INLINE ezMaterialResourceHandle GetMaterial(ezUInt32 uiIndex) const
  {
    if (uiIndex >= m_Materials.GetCount())
      return ezMaterialResourceHandle();

    return m_Materials[uiIndex];
  }

  EZ_FORCE_INLINE void SetRenderPass(ezRenderPassType renderpass)
  {
    m_iRenderPass = renderpass;
  }

  virtual ezResult OnAttachedToObject() override;
  virtual ezResult OnDetachedFromObject() override;

  void OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const;
  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  void SetMeshFile(const char* szFile);
  const char* GetMeshFile() const;

  ezColor m_MeshColor;

private:
  ezRenderPassType m_iRenderPass;
  ezMeshResourceHandle m_hMesh;
  ezDynamicArray<ezMaterialResourceHandle> m_Materials;
};

