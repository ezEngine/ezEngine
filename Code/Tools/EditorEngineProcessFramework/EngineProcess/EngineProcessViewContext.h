#pragma once

#include <EditorEngineProcessFramework/Plugin.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <System/Window/Window.h>
#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezEngineProcessDocumentContext;
class ezEditorEngineDocumentMsg;
class ezViewRedrawMsgToEngine;
class ezEditorEngineViewMsg;
class ezGALRenderTagetSetup;

typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;

/// \brief Represents the window inside the editor process, into which the engine process renders
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorProcessViewWindow : public ezWindowBase
{
public:
  ezEditorProcessViewWindow()
  {
    m_hWnd = 0;
    m_uiWidth = 0;
    m_uiHeight = 0;
  }


  // Inherited via ezWindowBase
  virtual ezSizeU32 GetClientAreaSize() const override { return ezSizeU32(m_uiWidth, m_uiHeight); }
  virtual ezWindowHandle GetNativeWindowHandle() const override { return m_hWnd; }
  virtual void ProcessWindowMessages() override { }
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override { return false; }

  ezWindowHandle m_hWnd;
  ezUInt16 m_uiWidth;
  ezUInt16 m_uiHeight;
};

/// \brief Represents the view/window on the engine process side, holds all data necessary for rendering
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEngineProcessViewContext
{
public:
  ezEngineProcessViewContext(ezEngineProcessDocumentContext* pContext);
  virtual ~ezEngineProcessViewContext();

  void SetViewID(ezUInt32 id);

  ezEngineProcessDocumentContext* GetDocumentContext() const { return m_pDocumentContext; }

  virtual void HandleViewMessage(const ezEditorEngineViewMsg* pMsg);
  virtual void SetupRenderTarget(ezGALRenderTagetSetup& renderTargetSetup, ezUInt16 uiWidth, ezUInt16 uiHeight);
  virtual void Redraw(bool bRenderEditorGizmos);

  /// \brief Focuses camera on the given object
  static bool FocusCameraOnObject(ezCamera& camera, const ezBoundingBoxSphere& objectBounds, float fFov, const ezVec3& vViewDir);

  ezViewHandle GetViewHandle() const { return m_hView; }

protected:
  void SendViewMessage(ezEditorEngineViewMsg* pViewMsg);
  void HandleWindowUpdate(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight);
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg);

  /// \brief Returns the handle to the default render pipeline.
  virtual ezRenderPipelineResourceHandle CreateDefaultRenderPipeline();

  /// \brief Returns the handle to the debug render pipeline.
  virtual ezRenderPipelineResourceHandle CreateDebugRenderPipeline();

  /// \brief Create the actual view.
  virtual ezViewHandle CreateView() = 0;

private:
  ezEngineProcessDocumentContext* m_pDocumentContext;
  ezEditorProcessViewWindow m_Window;

protected:
  ezCamera m_Camera;
  ezViewHandle m_hView;
  ezUInt32 m_uiViewID;
};
