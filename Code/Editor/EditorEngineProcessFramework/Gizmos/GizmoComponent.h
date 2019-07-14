#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <RendererCore/Meshes/MeshComponent.h>

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGizmoRenderData : public ezMeshRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoRenderData, ezMeshRenderData);

public:
  ezColor m_GizmoColor;
  bool m_bUseDepthPrepass;
  bool m_bIsPickable;
};

class ezGizmoComponent;
class ezGizmoComponentManager : public ezComponentManager<ezGizmoComponent, ezBlockStorageType::FreeList>
{
public:
  ezGizmoComponentManager(ezWorld* pWorld);

  ezUInt32 m_uiHighlightID = 0;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGizmoComponent : public ezMeshComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezGizmoComponent, ezMeshComponent, ezGizmoComponentManager);

public:
  ezGizmoComponent();

  ezColor m_GizmoColor;
  bool m_bUseDepthPrepass;
  bool m_bIsPickable = true;

protected:
  virtual ezMeshRenderData* CreateRenderData() const override;

};

