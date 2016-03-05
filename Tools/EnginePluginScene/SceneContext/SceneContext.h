#pragma once

#include <EnginePluginScene/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureResource.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;
class ezGameState;

class EZ_ENGINEPLUGINSCENE_DLL ezSceneContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneContext, ezEngineProcessDocumentContext);

public:
  ezSceneContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  const ezDeque<ezGameObjectHandle>& GetSelection() const { return m_Selection; }
  const ezDeque<ezGameObjectHandle>& GetSelectionWithChildren() const { return m_SelectionWithChildren; }
  bool GetRenderSelectionOverlay() const { return m_bRenderSelectionOverlay; }
  void RenderShapeIcons(ezRenderContext* pContext);
  void RenderSelectionBoxes(ezRenderContext* pContext);
  void GenerateShapeIconMesh();

  ezGameState* GetGameState() const;

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize();

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  
  void HandleSelectionMsg(const ezObjectSelectionMsgToEngine* pMsg);
  void HandleGameModeMsg(const ezGameModeMsgToEngine* pMsg);
  void ComputeHierarchyBounds(ezGameObject* pObj, ezBoundingBoxSphere& bounds);
  void ComputeSelectionBounds();
  void InsertSelectedChildren(const ezGameObject* pObject);
  void LoadShapeIconTextures();
  void CreateSelectionBoxMesh();
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);
  void SetSelectionTag(bool bAddTag);

  bool m_bRenderSelectionOverlay;
  bool m_bRenderShapeIcons;
  bool m_bRenderSelectionBoxes;
  bool m_bShapeIconBufferValid;
  ezDeque<ezGameObjectHandle> m_Selection;
  ezDeque<ezBoundingBoxSphere> m_SelectionBBoxes;
  ezDeque<ezGameObjectHandle> m_SelectionWithChildren;
  ezSet<ezGameObjectHandle> m_SelectionWithChildrenSet;

  struct ShapeIconData
  {
    ezTextureResourceHandle m_hTexture;
    ezMeshBufferResourceHandle m_hMeshBuffer;

    struct PosID
    {
      ezVec3 pos;
      ezUInt32 id;
    };

    ezDeque<PosID> m_IconPositions;
  };

  ezHashTable<const ezRTTI*, ShapeIconData> m_ShapeIcons;

  ezShaderResourceHandle m_hShapeIconShader;
  static ezUInt32 s_uiShapeIconBufferCounter;

  ezShaderResourceHandle m_hSelectionBoxShader;
  ezMeshBufferResourceHandle m_hSelectionBoxMeshBuffer;
};


