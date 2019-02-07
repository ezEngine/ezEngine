#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <ThirdParty/Imgui/imgui.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

struct ImGuiContext;

typedef ezDelegate<void(ImFontAtlas&)> ezImguiConfigFontCallback;
typedef ezDelegate<void(ImGuiStyle&)> ezImguiConfigStyleCallback;

/// \brief Singleton class through which one can control the third-party library 'Dear Imgui'
///
/// Instance has to be manually created and destroyed. Do this for example in ezGameState::OnActivation()
/// and ezGameState::OnDeactivation().
/// You need to call SetCurrentContextForView before you can use the Imgui functions directly.
/// E.g. 'ImGui::Text("Hello, world!");'
/// To prevent Imgui from using mouse and keyboard input (but still do rendering) use SetPassInputToImgui().
/// To prevent you app from using mouse and keyboard input when Imgui has focus, query WantsInput().
///
/// \note Don't forget that to see the GUI on screen, your render pipeline must contain an ezImguiExtractor
/// and you need to have an ezImguiRenderer set (typically on an ezSimpleRenderPass).
class EZ_GAMEENGINE_DLL ezImgui
{
  EZ_DECLARE_SINGLETON(ezImgui);

public:
  ezImgui(ezImguiConfigFontCallback configFontCallback = ezImguiConfigFontCallback(),
      ezImguiConfigStyleCallback configStyleCallback = ezImguiConfigStyleCallback());
  ~ezImgui();

  /// \brief Sets the ImGui context for the given view
  void SetCurrentContextForView(const ezViewHandle& hView);

  /// \brief Returns the value that was passed to BeginFrame(). Useful for positioning UI elements.
  ezSizeU32 GetCurrentWindowResolution() const { return m_CurrentWindowResolution; }

  /// \brief When this is disabled, the GUI will be rendered, but it will not react to any input. Useful if something else shall get
  /// exclusive input.
  void SetPassInputToImgui(bool bPassInput) { m_bPassInputToImgui = bPassInput; }

  /// \brief If this returns true, the GUI wants to use the input, and thus you might want to not use the input for anything else.
  ///
  /// This is the case when the mouse hovers over any window or a text field has keyboard focus.
  bool WantsInput() const { return m_bImguiWantsInput; }

  /// \brief Returns the shared font atlas
  ImFontAtlas& GetFontAtlas() { return *m_pSharedFontAtlas; }

private:
  friend class ezImguiExtractor;
  friend class ezImguiRenderer;

  void Startup(ezImguiConfigFontCallback configFontCallback);
  void Shutdown();

  ImGuiContext* CreateContext();
  void BeginFrame(const ezViewHandle& hView);

  bool m_bPassInputToImgui = true;
  bool m_bImguiWantsInput = false;
  ezSizeU32 m_CurrentWindowResolution;
  ezHybridArray<ezTexture2DResourceHandle, 4> m_hTextures;

  ezImguiConfigStyleCallback m_ConfigStyleCallback;

  ezUniquePtr<ImFontAtlas> m_pSharedFontAtlas;

  struct Context
  {
    ImGuiContext* m_pImGuiContext = nullptr;
    ezUInt64 m_uiFrameBeginCounter = -1;
    ezUInt64 m_uiFrameRenderCounter = -1;
  };

  ezHashTable<ezViewHandle, Context> m_ViewToContextTable;
};

