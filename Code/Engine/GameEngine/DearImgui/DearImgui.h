#pragma once

#include <GameEngine/Basics.h>
#include <ThirdParty/Imgui/imgui.h>
#include <Foundation/Configuration/Singleton.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Math/Size.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

/// \brief Singleton class through which one can control the third-party library 'Dear Imgui'
///
/// Instance has to be manually created and destroyed. Do this for example in ezGameState::OnActivation()
/// and ezGameState::OnDeactivation().
/// Every frame you have to call ezImgui::BeginNewFrame(). After that you can use the Imgui functions directly.
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
  ezImgui();
  ~ezImgui();

  /// \brief This has to be called every frame before any Imgui calls are made (otherwise it asserts).
  ///
  /// The resolution is needed for proper GUI clipping. Use the ezWindow client area.
  void BeginNewFrame(ezSizeU32 windowResolution);

  /// \brief Returns the value that was passed to BeginFrame(). Useful for positioning UI elements.
  ezSizeU32 GetCurrentWindowResolution() const { return m_CurrentWindowResolution;}

  /// \brief When this is disabled, the GUI will be rendered, but it will not react to any input. Useful if something else shall get exclusive input.
  void SetPassInputToImgui(bool bPassInput) { m_bPassInputToImgui = bPassInput; }

  /// \brief If this returns true, the GUI wants to use the input, and thus you might want to not use the input for anything else.
  ///
  /// This is the case when the mouse hovers over any window or a text field has keyboard focus.
  bool WantsInput() const { return m_bImguiWantsInput; }

  /// \brief For internal use by the renderer.
  const ezHybridArray<ezTexture2DResourceHandle, 4>& GetTextures() const { return m_hTextures; }

private:
  void Startup();
  void Shutdown();

  bool m_bPassInputToImgui = true;
  bool m_bImguiWantsInput = false;
  ezSizeU32 m_CurrentWindowResolution;
  ezHybridArray<ezTexture2DResourceHandle, 4> m_hTextures;
};

