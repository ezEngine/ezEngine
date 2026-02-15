#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

class ezGALDevice;
class ezEngineViewWindow;
struct ezEditorAppEvent;

/// \brief Manages the editor-side rendering device for shared texture view rendering.
class EZ_EDITORFRAMEWORK_DLL ezEditorRendererSubsystem
{
public:
  static ezGALDevice* GetDevice();
  static void RegisterViewWindow(ezEngineViewWindow* pWindow);
  static void UnregisterViewWindow(ezEngineViewWindow* pWindow);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, EditorRendererSubsystem);
  static void EditorAppEventHandler(const ezEditorAppEvent& e);
  static void CreateDevice();
  static void DestroyDevice();

  static ezGALDevice* s_pDevice;
};
