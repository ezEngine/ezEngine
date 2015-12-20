#pragma once

#include <EnginePluginScene/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ShaderResource.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;

class EZ_ENGINEPLUGINSCENE_DLL ezSceneContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneContext, ezEngineProcessDocumentContext);

public:
  ezSceneContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  const ezDeque<ezGameObjectHandle>& GetSelection() const { return m_Selection; }
  const ezDeque<ezGameObjectHandle>& GetSelectionWithChildren() const { return m_SelectionWithChildren; }
  bool GetRenderSelectionOverlay() const { return m_bRenderSelectionOverlay; }
  bool GetRenderShapeIcons() const { return m_bRenderShapeIcons; }
  void GenerateShapeIconMesh();
  void RenderShapeIcons(ezRenderContext* pContext);

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize();

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  void HandleSelectionMsg(const ezObjectSelectionMsgToEngine* pMsg);
  void ComputeHierarchyBounds(ezGameObject* pObj, ezBoundingBoxSphere& bounds);
  void InsertSelectedChildren(const ezGameObject* pObject);

  bool m_bRenderSelectionOverlay;
  bool m_bRenderShapeIcons;
  bool m_bShapeIconBufferValid;
  ezDeque<ezGameObjectHandle> m_Selection;
  ezDeque<ezGameObjectHandle> m_SelectionWithChildren;
  ezSet<ezGameObjectHandle> m_SelectionWithChildrenSet;

  ezUInt32 m_uiNumShapeIcons;
  ezMeshBufferResourceHandle m_hShapeIcons;
  ezShaderResourceHandle m_hShapeIconShader;
  ezUInt32 m_uiShapeIconBufferCounter;
};


