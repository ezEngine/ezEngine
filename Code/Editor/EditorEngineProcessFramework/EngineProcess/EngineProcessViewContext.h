#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <RendererCore/Pipeline/Declarations.h>

class ezEngineProcessDocumentContext;
class ezEditorEngineDocumentMsg;
class ezViewRedrawMsgToEngine;
class ezEditorEngineViewMsg;
struct ezGALRenderTargets;

using ezRenderPipelineResourceHandle = ezTypedResourceHandle<class ezRenderPipelineResource>;

/// \brief Represents the window inside the editor process, into which the engine process renders
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorProcessViewWindow : public ezWindowBase
{
public:
  ezEditorProcessViewWindow()
  {
    m_hWnd = INVALID_WINDOW_HANDLE_VALUE;
    m_uiWidth = 0;
    m_uiHeight = 0;
  }

  ~ezEditorProcessViewWindow();

  ezResult UpdateWindow(ezWindowHandle hParentWindow, ezUInt16 uiWidth, ezUInt16 uiHeight);

  // Inherited via ezWindowBase
  virtual ezSizeU32 GetClientAreaSize() const override { return ezSizeU32(m_uiWidth, m_uiHeight); }
  virtual ezWindowHandle GetNativeWindowHandle() const override { return m_hWnd; }
  virtual void ProcessWindowMessages() override {}
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override { return false; }
  virtual bool IsVisible() const override { return true; }
  virtual void AddReference() override { m_iReferenceCount.Increment(); }
  virtual void RemoveReference() override { m_iReferenceCount.Decrement(); }


  ezUInt16 m_uiWidth;
  ezUInt16 m_uiHeight;

private:
  ezWindowHandle m_hWnd;
  ezAtomicInteger32 m_iReferenceCount = 0;
};

/// \brief Represents the view/window on the engine process side, holds all data necessary for rendering
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEngineProcessViewContext
{
public:
  ezEngineProcessViewContext(ezEngineProcessDocumentContext* pContext);
  virtual ~ezEngineProcessViewContext();

  void SetViewID(ezUInt32 uiId);

  ezEngineProcessDocumentContext* GetDocumentContext() const { return m_pDocumentContext; }

  virtual void HandleViewMessage(const ezEditorEngineViewMsg* pMsg);
  virtual void SetupRenderTarget(ezGALSwapChainHandle hSwapChain, const ezGALRenderTargets* pRenderTargets, ezUInt16 uiWidth, ezUInt16 uiHeight);
  virtual void Redraw(bool bRenderEditorGizmos);

  /// \brief Focuses camera on the given object
  static bool FocusCameraOnObject(ezCamera& inout_camera, const ezBoundingBoxSphere& objectBounds, float fFov, const ezVec3& vViewDir);

  ezViewHandle GetViewHandle() const { return m_hView; }

  void DrawSimpleGrid() const;

protected:
  void SendViewMessage(ezEditorEngineViewMsg* pViewMsg);
  void HandleWindowUpdate(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight);
  void OnSwapChainChanged(ezGALSwapChainHandle hSwapChain, ezSizeU32 size);

  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg);

  /// \brief Returns the handle to the default render pipeline.
  virtual ezRenderPipelineResourceHandle CreateDefaultRenderPipeline();

  /// \brief Returns the handle to the debug render pipeline.
  virtual ezRenderPipelineResourceHandle CreateDebugRenderPipeline();

  /// \brief Create the actual view.
  virtual ezViewHandle CreateView() = 0;

private:
  ezEngineProcessDocumentContext* m_pDocumentContext;
  ezRegisteredWndHandle m_hEditorWindow;

protected:
  ezCamera m_Camera;
  ezViewHandle m_hView;
  ezUInt32 m_uiViewID;
};
