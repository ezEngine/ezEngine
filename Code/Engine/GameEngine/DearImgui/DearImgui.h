#pragma once

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/ResourceManager/ResourceHandle.h>
#  include <Foundation/Configuration/CVar.h>
#  include <Foundation/Configuration/Singleton.h>
#  include <Foundation/Containers/IdTable.h>
#  include <Foundation/Math/Size.h>
#  include <Foundation/Memory/CommonAllocators.h>
#  include <Foundation/Types/UniquePtr.h>
#  include <GameEngine/GameEngineDLL.h>
#  include <Imgui/imgui.h>
#  include <RendererCore/Pipeline/Declarations.h>

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;
using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;

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

  /// \brief When this is disabled, the GUI will be rendered, but it will not react to any input.
  ///
  /// Useful if something else shall get exclusive input.
  /// Be aware that this is global state, that affects ALL ImGui elements for the entire frame.
  void SetPassInputToImgui(bool bPassInput) { m_bPassInputToImgui = bPassInput; }
  bool GetPassInputToImgui() const { return m_bPassInputToImgui; }

  /// \brief If this returns true, the GUI wants to use the input, and thus you might want to not use the input for anything else.
  ///
  /// This is the case when the mouse hovers over any window or a text field has keyboard focus.
  bool WantsInput() const { return m_bImguiWantsInput; }

  /// \brief Returns the shared font atlas
  ImFontAtlas& GetFontAtlas() { return *m_pSharedFontAtlas; }

  /// Registers a texture resource and returns an ImTextureID that can be used with raw ImGui image calls or passed to RegisterImage(). The texture stays registered until UnregisterResource() is called.
  ImTextureID RegisterTexture(const ezTexture2DResourceHandle& hTexture);

  /// \copydoc RegisterTexture(const ezTexture2DResourceHandle&)
  ImTextureID RegisterTexture(ezGALTextureHandle hTexture);

  /// Registers a material resource and returns an ImTextureID. Works like RegisterTexture() but renders using the full material rather than just a texture.
  ImTextureID RegisterMaterial(const ezMaterialResourceHandle& hMaterial);

  /// Removes a previously registered texture or material. The ImTextureID must not be used afterwards.
  void UnregisterResource(ImTextureID id);

  struct Image
  {
    ImTextureID m_Id;
    ezVec2 m_UV0;
    ezVec2 m_UV1;
  };

  /// Associates a named image with a registered texture and UV coordinates. This allows the AddImage() and AddImageButton() convenience functions to look up the image by name instead of requiring the caller to pass the ImTextureID and UVs every time. Call RegisterTexture() first to obtain the texture ID.
  void RegisterImage(ezTempHashedString sImgId, ImTextureID texId, const ezVec2& vUv0, const ezVec2& vUv1);

  /// Renders a clickable image button using a previously registered image name.
  bool AddImageButton(ezTempHashedString sImgId, const char* szImguiID, const ezVec2& vImageSize, const ezColor& backgroundColor = ezColor::MakeZero(), const ezColor& tintColor = ezColor::White) const;

  /// Renders an image using a previously registered image name.
  void AddImage(ezTempHashedString sImgId, const ezVec2& vImageSize, const ezColor& tintColor = ezColor::White, const ezColor& borderColor = ezColor::MakeZero()) const;

  /// Like AddImageButton(), but draws a colored overlay from \a fProgress (0 to 1) to the right edge, which can be used to indicate a loading progress.
  bool AddImageButtonWithProgress(ezTempHashedString sImgId, const char* szImguiID, const ezVec2& vImageSize, float fProgress, const ezColor& overlayColor, const ezColor& tintColor = ezColor::White) const;

  /// Like AddImage(), but draws a colored overlay from \a fProgress (0 to 1) to the right edge, which can be used to indicate a loading progress.
  void AddImageWithProgress(ezTempHashedString sImgId, const char* szImguiID, const ezVec2& vImageSize, float fProgress, const ezColor& overlayColor, const ezColor& tintColor = ezColor::White) const;

private:
  friend class ezImguiExtractor;
  friend class ezImguiRenderer;

  using ezImGuiTextureIdData = ezGenericId<16, 16>;

  struct ezImGuiTextureRegistration
  {
    enum class Type : ezUInt8
    {
      Texture2D,
      GALTexture,
      Material,
    };

    Type m_Type = Type::Texture2D;
    ezTexture2DResourceHandle m_hTexture2D;
    ezGALTextureHandle m_hGALTexture;
    ezMaterialResourceHandle m_hMaterial;
  };

  void Startup(ezImguiConfigFontCallback configFontCallback);
  void Shutdown();

  ImGuiContext* CreateContext();
  void BeginFrame(const ezViewHandle& hView);
  void GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e);

  ezProxyAllocator m_Allocator;

  bool m_bPassInputToImgui = true;
  bool m_bImguiWantsInput = false;
  ezSizeU32 m_CurrentWindowResolution;
  ezIdTable<ezImGuiTextureIdData, ezImGuiTextureRegistration> m_RegisteredTextures;

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
  ezCVarFloat* m_pTextScaleCVar = nullptr;
};

#endif
