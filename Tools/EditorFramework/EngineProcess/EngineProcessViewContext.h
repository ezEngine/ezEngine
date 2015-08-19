#pragma once

#include <EditorFramework/Plugin.h>
#include <System/Window/Window.h>
#include <RendererFoundation/Device/Device.h>

class ezEngineProcessDocumentContext;
class ezEditorEngineViewMsg;

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
  virtual void ProcessWindowMessages() override { }

  ezWindowHandle m_hWnd;
  ezUInt16 m_uiWidth;
  ezUInt16 m_uiHeight;
};

/// \brief Represents the view/window on the engine process side, holds all data necessary for rendering
class EZ_EDITORFRAMEWORK_DLL ezEngineProcessViewContext
{
public:
  ezEngineProcessViewContext(ezEngineProcessDocumentContext* pContext) : m_pDocumentContext(pContext) { }
  virtual ~ezEngineProcessViewContext() { }

  ezEditorProcessViewWindow& GetEditorWindow() { return m_Window; }

  ezEngineProcessDocumentContext* GetDocumentContext() const { return m_pDocumentContext; }

  virtual void HandleViewMessage(const ezEditorEngineViewMsg* pMsg) = 0;

private:
  ezEngineProcessDocumentContext* m_pDocumentContext;
  ezEditorProcessViewWindow m_Window;

};


