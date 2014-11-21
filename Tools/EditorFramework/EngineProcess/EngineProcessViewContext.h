#pragma once

#include <EditorFramework/EditorApp.moc.h>
#include <System/Window/Window.h>
#include <RendererFoundation/Device/Device.h>

/// \brief Represents the window inside the editor process, into which the engine process renders
class EZ_EDITORFRAMEWORK_DLL ezEditorProcessViewWindow : public ezWindowBase
{
public:
  ezEditorProcessViewWindow()
  {
    m_hWnd = 0;
    m_uiWidth = 0;
    m_uiHeight = 0;
  }

  virtual ezSizeU32 GetClientAreaSize() const override { return ezSizeU32(m_uiWidth, m_uiHeight); }
  virtual ezWindowHandle GetNativeWindowHandle() const override { return m_hWnd; }

  ezWindowHandle m_hWnd;
  ezUInt16 m_uiWidth;
  ezUInt16 m_uiHeight;
};

/// \brief Represents the view/window on the engine process side, holds all data necessary for rendering
class EZ_EDITORFRAMEWORK_DLL ezEngineProcessViewContext
{
public:
  ezEngineProcessViewContext(ezInt32 iViewIndex, ezUuid DocumentGuid) : m_iViewIndex(iViewIndex), m_DocumentGuid(DocumentGuid) { }
  virtual ~ezEngineProcessViewContext() { }

  ezEditorProcessViewWindow& GetEditorWindow() { return m_Window; }

  ezInt32 GetViewIndex() const { return m_iViewIndex; }
  ezUuid GetDocumentGuid() const { return m_DocumentGuid; }

  static ezEngineProcessViewContext* GetViewContext(ezUInt32 uiViewID);
  static void AddViewContext(ezUInt32 uiViewID, ezEngineProcessViewContext* pView);
  static void DestroyViewContext(ezUInt32 uiViewID);

private:
  ezInt32 m_iViewIndex;
  ezUuid m_DocumentGuid;
  ezEditorProcessViewWindow m_Window;

  static ezHashTable<ezUInt32, ezEngineProcessViewContext*> s_ViewContexts;
};


