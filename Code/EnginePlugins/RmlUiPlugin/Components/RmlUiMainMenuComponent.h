#pragma once

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/System/Window.h>
#include <Foundation/System/Screen.h>
#include <GameEngine/Configuration/InputRebinding.h>
#include <GameEngine/UI/MainMenuComponent.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>

namespace Rml
{
  class Element;
  class ElementDocument;
  class Event;
}

/// One volume slider on the 'Audio' tab of ezRmlUiMainMenuComponent.
///
/// What a sound group name means depends on the sound plugin: ezMiniAudio uses the names from its sound
/// group configuration, FMOD expects the GUID of a VCA, which is why the label is configured separately.
struct EZ_RMLUIPLUGIN_DLL ezRmlUiSoundGroupSetting
{
  ezString m_sLabel;
  ezString m_sSoundGroup;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RMLUIPLUGIN_DLL, ezRmlUiSoundGroupSetting);

//////////////////////////////////////////////////////////////////////////

/// Updates in PostAsync, so that it always runs before the canvases (PostTransform). Text that is changed after a canvas
/// updated its RmlUi context is rendered without a layout, ie. not at all.
using ezRmlUiMainMenuComponentManager = ezComponentManagerSimple<class ezRmlUiMainMenuComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::Compact, ezWorldUpdatePhase::PostAsync>;

/// A ready made main menu with a settings page, implemented with RmlUi.
///
/// ezFallbackGameState opens it when the user presses ESC, instead of quitting the application.
/// Custom game states can open it through the ezMainMenuComponent interface. The menu closes itself.
///
/// The component creates child objects with the canvases for the two pages. The documents can be replaced,
/// but the element IDs and event names of the built-in documents have to be kept, since the code addresses the widgets by those.
///
/// The settings are stored in CVars whose names start with 'Options.' and are applied to the engine when the
/// component is activated. A project whose scenes don't contain this component therefore starts with the engine's defaults.
///
/// Changing the window resolution or mode at runtime requires ezWindowPlatformShared::Reconfigure(), which is only
/// implemented on Windows. Elsewhere the display settings take effect after a restart.
///
/// The 'Controls' tab is implemented with ezInputRebinding, which a custom menu can use as well.
class EZ_RMLUIPLUGIN_DLL ezRmlUiMainMenuComponent : public ezMainMenuComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRmlUiMainMenuComponent, ezMainMenuComponent, ezRmlUiMainMenuComponentManager);

public:
  ezRmlUiMainMenuComponent();
  ~ezRmlUiMainMenuComponent();

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  void Update();

  virtual void OpenMenu() override;
  virtual bool IsMenuOpen() const override { return m_Page != Page::Closed; }

  EZ_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(MenuFile, m_hMenuResource, SetMenuResource);
  void SetMenuResource(const ezRmlUiResourceHandle& hResource);                  // [ property ]
  const ezRmlUiResourceHandle& GetMenuResource() const { return m_hMenuResource; } // [ property ]

  EZ_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(SettingsFile, m_hSettingsResource, SetSettingsResource);
  void SetSettingsResource(const ezRmlUiResourceHandle& hResource);                      // [ property ]
  const ezRmlUiResourceHandle& GetSettingsResource() const { return m_hSettingsResource; } // [ property ]

  /// Whether the world's clock is paused while the menu is open. A world that was already paused stays paused.
  void SetPauseWorld(bool bPause);                  // [ property ]
  bool GetPauseWorld() const { return m_bPauseWorld; } // [ property ]

  /// One volume slider is shown on the 'Audio' tab for each entry, below the master volume.
  ezDynamicArray<ezRmlUiSoundGroupSetting> m_SoundGroups; // [ property ]

  /// The input sets whose actions can be rebound on the 'Controls' tab.
  ///
  /// When empty, every input set is offered, except the ones that the engine registers for itself, see ezInputRebinding::IsEngineInputSet().
  ezDynamicArray<ezString> m_InputSets; // [ property ]

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  enum class Page
  {
    Closed,
    Menu,
    Settings,
  };

  void SetMenuPage(Page page);

  /// Component updates and the RmlUi event handlers run on a worker thread. Everything that has to happen on the
  /// main thread (window changes, mouse cursor, polling input) is done here, at the start of the application tick.
  void OnExecutionEvent(const ezGameApplicationExecutionEvent& e);
  void UnregisterExecutionEvents();

  void HandleEscape();
  void TickMainThread();

  ezRmlUiCanvas2DComponent* CreateCanvas(const char* szName, const ezRmlUiResourceHandle& hResource, ezTypedComponentHandle<ezRmlUiCanvas2DComponent>& out_hCanvas);
  ezRmlUiCanvas2DComponent* GetCanvas(const ezTypedComponentHandle<ezRmlUiCanvas2DComponent>& hCanvas);

  void RegisterMenuEventHandlers(ezRmlUiContext* pContext);
  void RegisterSettingsEventHandlers(ezRmlUiContext* pContext);

  /// Done once when the page is opened, rather than every frame, because writing to a dropdown while it is open interferes with the selection.
  void InitializeWidgetsFromSettings(Rml::ElementDocument* pDocument);
  void RebuildSoundGroupRows(Rml::ElementDocument* pDocument);
  void UpdateValueLabels(Rml::ElementDocument* pDocument);

  void RebuildInputBindingRows(Rml::ElementDocument* pDocument);
  void UpdateInputBindingLabels(Rml::ElementDocument* pDocument);

  /// Resets the settings of the tab that is currently shown.
  void RestoreDefaultSettings(Rml::ElementDocument* pDocument);

  void LoadSoundGroupVolumes();

  /// m_SoundGroups is a property and may change while the component is active, e.g. when it is edited during play-the-game.
  void SyncSoundGroupVolumes();
  void SaveSoundGroupVolumes() const;

  void UpdateStats(Rml::ElementDocument* pDocument);

  void RebuildMonitorList(Rml::ElementDocument* pDocument);
  void RebuildResolutionList(Rml::ElementDocument* pDocument);
  void UpdateResolutionEnabledState(Rml::ElementDocument* pDocument);

  /// Must be called from the main thread, resizing the window from a worker thread deadlocks.
  void ApplyDisplaySettings();

  void RequestApplySettings() { m_bApplySettings = true; }

  /// Maps the 'Options.' CVars to the engine's own CVars and systems. Must be called from the main thread.
  void ApplySettings();

  Page m_Page = Page::Closed;

  ezEventSubscriptionID m_ExecutionEventsId = 0;

  /// Set by OpenMenu(), so that the ESC press that opened the menu doesn't close it again in the next tick.
  bool m_bIgnoreEscape = false;

  ezRmlUiResourceHandle m_hMenuResource;
  ezRmlUiResourceHandle m_hSettingsResource;

  ezTypedComponentHandle<ezRmlUiCanvas2DComponent> m_hMenuCanvas;
  ezTypedComponentHandle<ezRmlUiCanvas2DComponent> m_hSettingsCanvas;

  /// The volume of each entry in m_SoundGroups.
  ezDynamicArray<float> m_GroupVolumes;

  bool m_bFocusPending = false;
  bool m_bPauseWorld = true;
  bool m_bClockWasPaused = false;
  bool m_bWidgetsInitialized = false;
  bool m_bUpdateValueLabels = false;
  bool m_bApplySettings = false;
  bool m_bApplyDisplaySettings = false;
  bool m_bMenuHandlersRegistered = false;
  bool m_bSettingsHandlersRegistered = false;

  ezUInt32 m_uiCursorOverrideId = 0;

  ezInputRebinding m_InputRebinding;
  bool m_bUpdateBindingLabels = false;

  ezDynamicArray<ezScreenInfo> m_Screens;
  ezDynamicArray<ezSizeU32> m_Resolutions;

  /// The display settings as edited in the UI, only saved and applied when 'Apply' is pressed.
  ezWindowCreationDesc m_PendingWindowDesc;
  ezString m_sDisplayStatus;
};
