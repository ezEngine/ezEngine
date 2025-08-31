#pragma once

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/ResourceManager/ResourceHandle.h>
#  include <Foundation/Configuration/Singleton.h>
#  include <Foundation/Math/Size.h>
#  include <Foundation/Memory/CommonAllocators.h>
#  include <Foundation/Types/UniquePtr.h>
#  include <GameEngine/GameEngineDLL.h>
#  include <RendererCore/Pipeline/Declarations.h>

#  include <Imgui/imgui.h>

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;

struct ImGuiContext;
struct ezGameApplicationExecutionEvent;

using ezImguiConfigFontCallback = ezDelegate<void(ImFontAtlas&)>;
using ezImguiConfigStyleCallback = ezDelegate<void(ImGuiStyle&)>;

/// \brief Singleton class through which one can control the third-party library 'Dear Imgui'
///
/// Instance has to be manually created and destroyed. Do this for example in ezGameState::OnActivation()
/// and ezGameState::OnDeactivation().
/// You need to call SetCurrentContextForView before you can use the Imgui functions directly.
/// E.g. 'ImGui::Text("Hello, world!");'
/// To prevent Imgui from using mouse and keyboard input (but still do rendering) use SetPassInputToImgui().
/// To prevent your app from using mouse and keyboard input when Imgui has focus, query WantsInput().
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

  ImTextureID RegisterTexture(const ezTexture2DResourceHandle& hTexture);

  struct Image
  {
    ImTextureID m_Id;
    ezVec2 m_UV0;
    ezVec2 m_UV1;
  };

  void RegisterImage(ezTempHashedString sImgId, ImTextureID pTexId, const ezVec2& vUv0, const ezVec2& vUv1);

  bool AddImageButton(ezTempHashedString sImgId, const char* szImguiID, const ezVec2& vImageSize, const ezColor& backgroundColor = ezColor::MakeZero(), const ezColor& tintColor = ezColor::White) const;

  void AddImage(ezTempHashedString sImgId, const ezVec2& vImageSize, const ezColor& tintColor = ezColor::White, const ezColor& borderColor = ezColor::MakeZero()) const;

  bool AddImageButtonWithProgress(ezTempHashedString sImgId, const char* szImguiID, const ezVec2& vImageSize, float fProgress, const ezColor& overlayColor, const ezColor& tintColor = ezColor::White) const;

  void AddImageWithProgress(ezTempHashedString sImgId, const char* szImguiID, const ezVec2& vImageSize, float fProgress, const ezColor& overlayColor, const ezColor& tintColor = ezColor::White) const;

private:
  friend class ezImguiExtractor;
  friend class ezImguiRenderer;

  void Startup(ezImguiConfigFontCallback configFontCallback);
  void Shutdown();

  ImGuiContext* CreateContext();
  void BeginFrame(const ezViewHandle& hView);
  void GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e);

  ezProxyAllocator m_Allocator;

  bool m_bPassInputToImgui = true;
  bool m_bImguiWantsInput = false;
  ezSizeU32 m_CurrentWindowResolution;
  ezHybridArray<ezTexture2DResourceHandle, 4> m_Textures;

  ezImguiConfigStyleCallback m_ConfigStyleCallback;

  ezUniquePtr<ImFontAtlas> m_pSharedFontAtlas;

  struct Context
  {
    ImGuiContext* m_pImGuiContext = nullptr;
    ezUInt64 m_uiFrameBeginCounter = -1;
    ezUInt64 m_uiFrameRenderCounter = -1;
  };

  ezMutex m_ViewToContextTableMutex;
  ezHashTable<ezViewHandle, Context> m_ViewToContextTable;
  ezHashTable<ezTempHashedString, Image> m_Images;
};

#endif
