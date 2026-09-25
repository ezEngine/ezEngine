#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/Interfaces/SoundInterface.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/System/Screen.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/GameState.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RmlUiPlugin/Components/RmlUiMainMenuComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiUtils.h>

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <RmlUi/Core/Event.h>

// defined in ShadowPool.cpp, which has no public header for them
EZ_RENDERERCORE_DLL extern ezCVarInt cvar_RenderingShadowsAtlasSize;
EZ_RENDERERCORE_DLL extern ezCVarInt cvar_RenderingShadowsMaxShadowMapSize;

// The user's choices, saved across runs. These are separate from the engine's CVars, because a single choice
// (e.g. 'Medium') maps to several engine values, see ezRmlUiMainMenuComponent::ApplySettings().
ezCVarInt cvar_OptionsTextureFiltering("Options.Graphics.TextureFiltering", 4, ezCVarFlags::Save, "Texture filtering, from 0 (nearest) to 6 (anisotropic 16x). Matches ezGALTextureQuality.");
ezCVarInt cvar_OptionsTextureQuality("Options.Graphics.TextureQuality", 5, ezCVarFlags::Save, "Texture quality, from 0 (very low) to 5 (ultra).");
ezCVarInt cvar_OptionsShadowQuality("Options.Graphics.ShadowQuality", 2, ezCVarFlags::Save, "Shadow quality, from 0 (low) to 3 (ultra).");
ezCVarFloat cvar_OptionsMasterVolume("Options.Audio.MasterVolume", 1.0f, ezCVarFlags::Save, "Volume of all sounds, from 0 to 1.");
// The sound groups are only known once a scene is loaded, but CVars must exist before the saved values are read, so all go into one CVar.
ezCVarString cvar_OptionsSoundGroupVolumes("Options.Audio.SoundGroupVolumes", "", ezCVarFlags::Save, "Volumes of the individual sound groups, as a ';' separated list of 'group=volume' pairs.");
ezCVarFloat cvar_OptionsUiScale("Options.UI.Scale", 1.0f, ezCVarFlags::Save, "Size of the menu, 1 being the size that the UI documents specify.");
// Not used by the engine, it demonstrates a text field.
ezCVarString cvar_OptionsPlayerName("Options.Game.PlayerName", "Player", ezCVarFlags::Save, "The name that the player chose.");

namespace
{
  constexpr const char* s_szDefaultMenuDocument = "{ 1c5c6a91-80d3-ccce-3c89-ac2150700d87 }";     // rmlui-menu/ez-main-menu.ezRmlUiAsset
  constexpr const char* s_szDefaultSettingsDocument = "{ 31a4e443-c1eb-e15d-3bfc-6e5750ce191a }"; // rmlui-menu/ez-app-settings.ezRmlUiAsset

  // ezShadowPool clamps the max size to a power of two in [64, 2048] and the atlas size to a multiple of it in [max, 8192].
  struct ShadowQualityPreset
  {
    ezUInt32 m_uiMaxShadowMapSize;
    ezUInt32 m_uiAtlasSize;
  };

  constexpr ShadowQualityPreset s_ShadowQualityPresets[] = {
    {256, 2048},  // Low
    {512, 2048},  // Medium
    {1024, 4096}, // High (the engine default)
    {2048, 8192}, // Ultra
  };

  // Matches the radio buttons in the RML file. Exclusive fullscreen changes the display resolution, which can't
  // be applied to an existing window, so it isn't offered.
  constexpr ezWindowMode::Enum s_WindowModes[] = {
    ezWindowMode::WindowFixedResolution,
    ezWindowMode::FullscreenBorderlessNativeResolution,
  };

  // Matches the options in the RML file. Dropping mips affects every texture, the resolution limit only the large ones.
  struct TextureQualityPreset
  {
    ezUInt32 m_uiDropMips;
    ezUInt32 m_uiMaxResolution;
  };

  constexpr TextureQualityPreset s_TextureQualityPresets[] = {
    {3, 256},   // Very Low
    {2, 512},   // Low
    {1, 1024},  // Medium
    {0, 2048},  // High
    {0, 4096},  // Very High
    {0, 16384}, // Ultra
  };

  // only the Windows implementation of ezScreen reports the supported resolutions
  const ezSizeU32 s_FallbackResolutions[] = {
    ezSizeU32(1280, 720),
    ezSizeU32(1600, 900),
    ezSizeU32(1920, 1080),
    ezSizeU32(2560, 1440),
    ezSizeU32(3840, 2160),
  };

  ezWindow* GetMainWindow()
  {
    ezGameState* pGameState = ezGameState::GetActiveGameState();
    return pGameState != nullptr ? pGameState->GetMainWindow() : nullptr;
  }
} // namespace

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezRmlUiSoundGroupSetting, ezNoBase, 1, ezRTTIDefaultAllocator<ezRmlUiSoundGroupSetting>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Label", m_sLabel),
    EZ_MEMBER_PROPERTY("SoundGroup", m_sSoundGroup),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezRmlUiMainMenuComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("MenuFile", GetMenuResource, SetMenuResource)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Rml_UI"), new ezDefaultValueAttribute(s_szDefaultMenuDocument)),
    EZ_RESOURCE_ACCESSOR_PROPERTY("SettingsFile", GetSettingsResource, SetSettingsResource)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Rml_UI"), new ezDefaultValueAttribute(s_szDefaultSettingsDocument)),
    EZ_ACCESSOR_PROPERTY("PauseWorld", GetPauseWorld, SetPauseWorld)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ARRAY_MEMBER_PROPERTY("SoundGroups", m_SoundGroups),
    EZ_ARRAY_MEMBER_PROPERTY("InputSets", m_InputSets),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input/RmlUi"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezRmlUiMainMenuComponent::ezRmlUiMainMenuComponent()
{
  m_hMenuResource = ezResourceManager::LoadResource<ezRmlUiResource>(s_szDefaultMenuDocument);
  m_hSettingsResource = ezResourceManager::LoadResource<ezRmlUiResource>(s_szDefaultSettingsDocument);
}

ezRmlUiMainMenuComponent::~ezRmlUiMainMenuComponent() = default;

void ezRmlUiMainMenuComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hMenuResource;
  s << m_hSettingsResource;
  s << m_bPauseWorld;
  s << m_SoundGroups.GetCount();

  for (const auto& group : m_SoundGroups)
  {
    s << group.m_sLabel;
    s << group.m_sSoundGroup;
  }

  s << m_InputSets.GetCount();

  for (const auto& sSet : m_InputSets)
  {
    s << sSet;
  }
}

void ezRmlUiMainMenuComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s >> m_hMenuResource;
  s >> m_hSettingsResource;
  s >> m_bPauseWorld;

  ezUInt32 uiNumSoundGroups = 0;
  s >> uiNumSoundGroups;

  m_SoundGroups.SetCount(uiNumSoundGroups);
  for (auto& group : m_SoundGroups)
  {
    s >> group.m_sLabel;
    s >> group.m_sSoundGroup;
  }

  ezUInt32 uiNumInputSets = 0;
  s >> uiNumInputSets;

  m_InputSets.SetCount(uiNumInputSets);
  for (auto& sSet : m_InputSets)
  {
    s >> sSet;
  }
}

void ezRmlUiMainMenuComponent::SetMenuResource(const ezRmlUiResourceHandle& hResource)
{
  m_hMenuResource = hResource;
}

void ezRmlUiMainMenuComponent::SetSettingsResource(const ezRmlUiResourceHandle& hResource)
{
  m_hSettingsResource = hResource;
}

void ezRmlUiMainMenuComponent::SetPauseWorld(bool bPause)
{
  m_bPauseWorld = bPause;
}

ezRmlUiCanvas2DComponent* ezRmlUiMainMenuComponent::CreateCanvas(const char* szName, const ezRmlUiResourceHandle& hResource, ezTypedComponentHandle<ezRmlUiCanvas2DComponent>& out_hCanvas)
{
  ezGameObjectDesc desc;
  desc.m_sName.Assign(szName);
  desc.m_hParent = GetOwner()->GetHandle();

  ezGameObject* pObject = nullptr;
  GetWorld()->CreateObject(desc, pObject);

  ezRmlUiCanvas2DComponent* pCanvas = nullptr;
  out_hCanvas = ezRmlUiCanvas2DComponent::CreateComponent(pObject, pCanvas);

  pCanvas->SetRmlResource(hResource);
  pCanvas->SetPassInput(true);
  pCanvas->SetActiveFlag(false);

  return pCanvas;
}

ezRmlUiCanvas2DComponent* ezRmlUiMainMenuComponent::GetCanvas(const ezTypedComponentHandle<ezRmlUiCanvas2DComponent>& hCanvas)
{
  ezRmlUiCanvas2DComponent* pCanvas = nullptr;
  if (!GetWorld()->TryGetComponent(hCanvas, pCanvas))
    return nullptr;

  return pCanvas;
}

void ezRmlUiMainMenuComponent::OnActivated()
{
  SUPER::OnActivated();

  if (m_hMenuResource.IsValid())
  {
    CreateCanvas("MainMenuPage", m_hMenuResource, m_hMenuCanvas);
  }

  if (m_hSettingsResource.IsValid())
  {
    CreateCanvas("SettingsPage", m_hSettingsResource, m_hSettingsCanvas);
  }

  m_Page = Page::Closed;

  LoadSoundGroupVolumes();
  RequestApplySettings();
}

void ezRmlUiMainMenuComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // Not done in OnActivated(), because in the editor the scene is activated before the game state registers its input actions.
  m_InputRebinding.CollectActions(m_InputSets);
  ezInputRebinding::ApplyUserBindings();

  UnregisterExecutionEvents();

  if (ezGameApplicationBase* pApp = ezGameApplicationBase::GetGameApplicationBaseInstance())
  {
    m_ExecutionEventsId = pApp->m_ExecutionEvents.AddEventHandler(ezMakeDelegate(&ezRmlUiMainMenuComponent::OnExecutionEvent, this));
  }
}

void ezRmlUiMainMenuComponent::OnDeactivated()
{
  // the world must not stay paused
  SetMenuPage(Page::Closed);

  // OnSimulationStarted() is called again when the component gets reactivated
  UnregisterExecutionEvents();

  ezInputManager::PopMouseCursorOverride(m_uiCursorOverrideId);
  m_uiCursorOverrideId = 0;

  if (ezRmlUiCanvas2DComponent* pCanvas = GetCanvas(m_hMenuCanvas))
  {
    GetWorld()->DeleteObjectDelayed(pCanvas->GetOwner()->GetHandle());
  }

  if (ezRmlUiCanvas2DComponent* pCanvas = GetCanvas(m_hSettingsCanvas))
  {
    GetWorld()->DeleteObjectDelayed(pCanvas->GetOwner()->GetHandle());
  }

  m_hMenuCanvas.Invalidate();
  m_hSettingsCanvas.Invalidate();

  // the new canvases get new RmlUi contexts
  m_bMenuHandlersRegistered = false;
  m_bSettingsHandlersRegistered = false;

  SUPER::OnDeactivated();
}

void ezRmlUiMainMenuComponent::UnregisterExecutionEvents()
{
  if (m_ExecutionEventsId == 0)
    return;

  if (ezGameApplicationBase* pApp = ezGameApplicationBase::GetGameApplicationBaseInstance())
  {
    pApp->m_ExecutionEvents.RemoveEventHandler(m_ExecutionEventsId);
  }

  m_ExecutionEventsId = 0;
}

void ezRmlUiMainMenuComponent::OnExecutionEvent(const ezGameApplicationExecutionEvent& e)
{
  // BeginAppTick is sent from the main thread before the world update, later events may come from a worker thread
  if (e.m_Type != ezGameApplicationExecutionEvent::Type::BeginAppTick || !IsActive())
    return;

  EZ_LOCK(GetWorld()->GetWriteMarker());

  HandleEscape();
  TickMainThread();
}

void ezRmlUiMainMenuComponent::OpenMenu()
{
  if (IsMenuOpen())
    return;

  m_bIgnoreEscape = true;
  SetMenuPage(Page::Menu);
}

void ezRmlUiMainMenuComponent::HandleEscape()
{
  const bool bIgnore = m_bIgnoreEscape;
  m_bIgnoreEscape = false;

  // A raw slot, because the exclusive input set of the open menu suppresses all actions, and the ESC action only exists in development builds.
  // While a rebinding waits for a key, ESC cancels that instead.
  if (bIgnore || m_InputRebinding.IsCapturing() || ezInputManager::GetInputSlotState(ezInputSlot_KeyEscape) != ezKeyState::Pressed)
    return;

  switch (m_Page)
  {
    case Page::Closed:
      // opening the menu is up to the game state
      break;
    case Page::Menu:
      SetMenuPage(Page::Closed);
      break;
    case Page::Settings:
      SetMenuPage(Page::Menu);
      break;
  }
}

void ezRmlUiMainMenuComponent::SetMenuPage(Page page)
{
  if (m_Page == page)
    return;

  const bool bWasOpen = IsMenuOpen();

  m_Page = page;
  m_bFocusPending = (page != Page::Closed);

  if (ezRmlUiCanvas2DComponent* pCanvas = GetCanvas(m_hMenuCanvas))
  {
    pCanvas->SetActiveFlag(page == Page::Menu);
  }

  if (ezRmlUiCanvas2DComponent* pCanvas = GetCanvas(m_hSettingsCanvas))
  {
    pCanvas->SetActiveFlag(page == Page::Settings);

    if (page != Page::Settings)
    {
      // refilled when the page is opened again, to show what is configured at that point
      m_bWidgetsInitialized = false;

      m_InputRebinding.CancelCapture();

      if (m_InputRebinding.HasUnsavedChanges())
      {
        m_InputRebinding.SaveUserBindings();
      }
    }
  }

  if (m_bPauseWorld && bWasOpen != IsMenuOpen())
  {
    if (IsMenuOpen())
    {
      m_bClockWasPaused = GetWorld()->GetClock().GetPaused();
      GetWorld()->GetClock().SetPaused(true);
    }
    else
    {
      GetWorld()->GetClock().SetPaused(m_bClockWasPaused);
    }
  }
}

void ezRmlUiMainMenuComponent::ApplySettings()
{
  // The texture quality recreates sampler states, which deadlocks when done from the world update, which the event handlers run in.
  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "Use RequestApplySettings(), the settings can only be applied from the main thread.");

  const float fUiScale = ezMath::Clamp((float)cvar_OptionsUiScale, 0.5f, 2.0f);

  if (ezRmlUiCanvas2DComponent* pCanvas = GetCanvas(m_hMenuCanvas))
  {
    pCanvas->SetCustomScale(fUiScale);
  }

  if (ezRmlUiCanvas2DComponent* pCanvas = GetCanvas(m_hSettingsCanvas))
  {
    pCanvas->SetCustomScale(fUiScale);
  }

  ezRenderContext::GetDefaultInstance()->SetDefaultTextureQuality(
    static_cast<ezGALTextureQuality::Enum>(ezMath::Clamp<ezInt32>(cvar_OptionsTextureFiltering, 0, ezGALTextureQuality::Anisotropic16x)));

  {
    const auto& preset = s_TextureQualityPresets[ezMath::Clamp<ezInt32>(cvar_OptionsTextureQuality, 0, EZ_ARRAY_SIZE(s_TextureQualityPresets) - 1)];

    cvar_RenderingTexturesDropMips = preset.m_uiDropMips;
    cvar_RenderingTexturesMaxResolution = preset.m_uiMaxResolution;
  }

  {
    const auto& preset = s_ShadowQualityPresets[ezMath::Clamp<ezInt32>(cvar_OptionsShadowQuality, 0, EZ_ARRAY_SIZE(s_ShadowQualityPresets) - 1)];

    cvar_RenderingShadowsMaxShadowMapSize = preset.m_uiMaxShadowMapSize;
    cvar_RenderingShadowsAtlasSize = preset.m_uiAtlasSize;
  }

  SyncSoundGroupVolumes();

  // without a sound plugin the volumes are only remembered
  if (ezSoundInterface* pSound = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>())
  {
    pSound->SetMasterChannelVolume(ezMath::Clamp((float)cvar_OptionsMasterVolume, 0.0f, 1.0f));

    for (ezUInt32 i = 0; i < m_SoundGroups.GetCount(); ++i)
    {
      if (!m_SoundGroups[i].m_sSoundGroup.IsEmpty())
      {
        pSound->SetSoundGroupVolume(m_SoundGroups[i].m_sSoundGroup, ezMath::Clamp(m_GroupVolumes[i], 0.0f, 1.0f));
      }
    }
  }
}

void ezRmlUiMainMenuComponent::RegisterMenuEventHandlers(ezRmlUiContext* pContext)
{
  pContext->RegisterEventHandler("MainMenu-Resume", [this](Rml::Event& e)
    {
      SetMenuPage(Page::Closed);
      //
    });

  pContext->RegisterEventHandler("MainMenu-Settings", [this](Rml::Event& e)
    {
      SetMenuPage(Page::Settings);
      //
    });

  pContext->RegisterEventHandler("MainMenu-Exit", [](Rml::Event& e)
    {
      // in the editor this stops the play-the-game mode
      if (ezGameStateBase* pGameState = ezGameState::GetActiveGameState())
      {
        pGameState->RequestQuit("game");
      }
      //
    });
}

void ezRmlUiMainMenuComponent::RegisterSettingsEventHandlers(ezRmlUiContext* pContext)
{
  pContext->RegisterEventHandler("MainMenu-Back", [this](Rml::Event& e)
    {
      SetMenuPage(Page::Menu);
      //
    });

  pContext->RegisterEventHandler("toggle-vsync", [](Rml::Event& e)
    {
      ezGameApplication::cvar_AppVSync = e.GetParameter("checked", false);
      //
    });

  pContext->RegisterEventHandler("render-scale-change", [this](Rml::Event& e)
    {
      ezGameApplication::cvar_AppRenderScale = ezRmlUiUtils::GetChangedValue(e) / 100.0f;
      m_bUpdateValueLabels = true;
      //
    });

  pContext->RegisterEventHandler("toggle-fps", [](Rml::Event& e)
    {
      ezGameApplication::cvar_AppShowFPS = e.GetParameter("checked", false);
      //
    });

  pContext->RegisterEventHandler("ui-scale-change", [this](Rml::Event& e)
    {
      cvar_OptionsUiScale = ezRmlUiUtils::GetChangedValue(e) / 100.0f;
      RequestApplySettings();
      m_bUpdateValueLabels = true;
      //
    });

  pContext->RegisterEventHandler("texture-filtering-change", [this](Rml::Event& e)
    {
      const ezInt32 iSelection = static_cast<Rml::ElementFormControlSelect*>(e.GetCurrentElement())->GetSelection();

      if (iSelection >= 0)
      {
        cvar_OptionsTextureFiltering = iSelection;
        RequestApplySettings();
      }
      //
    });

  pContext->RegisterEventHandler("texture-quality-change", [this](Rml::Event& e)
    {
      const ezInt32 iSelection = static_cast<Rml::ElementFormControlSelect*>(e.GetCurrentElement())->GetSelection();

      if (iSelection >= 0)
      {
        cvar_OptionsTextureQuality = iSelection;
        RequestApplySettings();
      }
      //
    });

  pContext->RegisterEventHandler("shadow-quality-change", [this](Rml::Event& e)
    {
      const ezInt32 iSelection = static_cast<Rml::ElementFormControlSelect*>(e.GetCurrentElement())->GetSelection();

      if (iSelection >= 0)
      {
        cvar_OptionsShadowQuality = iSelection;
        RequestApplySettings();
      }
      //
    });

  pContext->RegisterEventHandler("master-volume-change", [this](Rml::Event& e)
    {
      cvar_OptionsMasterVolume = ezRmlUiUtils::GetChangedValue(e) / 100.0f;
      RequestApplySettings();
      m_bUpdateValueLabels = true;
      //
    });

  pContext->RegisterEventHandler("sound-group-volume-change", [this](Rml::Event& e)
    {
      // the element ID ends with the index, see RebuildSoundGroupRows()
      Rml::Element* pElement = e.GetCurrentElement();

      ezUInt32 uiIndex = 0;
      if (ezConversionUtils::StringToUInt(pElement->GetId().c_str() + ezStringUtils::GetStringElementCount("sound-group-"), uiIndex).Failed())
        return;

      if (uiIndex >= m_GroupVolumes.GetCount())
        return;

      m_GroupVolumes[uiIndex] = ezRmlUiUtils::GetChangedValue(e) / 100.0f;

      SaveSoundGroupVolumes();
      RequestApplySettings();
      m_bUpdateValueLabels = true;
      //
    });

  pContext->RegisterEventHandler("input-rebind", [this](Rml::Event& e)
    {
      // the element ID ends with '<binding>-<slot>', see RebuildInputBindingRows()
      const char* szIndices = e.GetCurrentElement()->GetId().c_str() + ezStringUtils::GetStringElementCount("input-bind-");
      const char* szSeparator = ezStringUtils::FindSubString(szIndices, "-");

      if (szSeparator == nullptr)
        return;

      ezUInt32 uiBinding = 0;
      ezUInt32 uiSlot = 0;

      if (ezConversionUtils::StringToUInt(ezStringView(szIndices, szSeparator), uiBinding).Failed() ||
          ezConversionUtils::StringToUInt(szSeparator + 1, uiSlot).Failed())
        return;

      m_InputRebinding.StartCapture(uiBinding, uiSlot);
      m_bUpdateBindingLabels = true;
      //
    });

  pContext->RegisterEventHandler("input-unbind", [this](Rml::Event& e)
    {
      ezUInt32 uiBinding = 0;
      if (ezConversionUtils::StringToUInt(e.GetCurrentElement()->GetId().c_str() + ezStringUtils::GetStringElementCount("input-unbind-"), uiBinding).Failed())
        return;

      m_InputRebinding.ClearBindings(uiBinding);
      m_bUpdateBindingLabels = true;
      //
    });

  pContext->RegisterEventHandler("input-restore", [this](Rml::Event& e)
    {
      ezUInt32 uiBinding = 0;
      if (ezConversionUtils::StringToUInt(e.GetCurrentElement()->GetId().c_str() + ezStringUtils::GetStringElementCount("input-restore-"), uiBinding).Failed())
        return;

      m_InputRebinding.RestoreDefault(uiBinding);
      m_bUpdateBindingLabels = true;
      //
    });

  pContext->RegisterEventHandler("player-name-change", [](Rml::Event& e)
    {
      cvar_OptionsPlayerName = e.GetParameter("value", Rml::String()).c_str();
      //
    });

  pContext->RegisterEventHandler("settings-defaults", [this](Rml::Event& e)
    {
      RestoreDefaultSettings(e.GetCurrentElement()->GetOwnerDocument());
      //
    });

  pContext->RegisterEventHandler("window-mode-change", [this](Rml::Event& e)
    {
      // the radio button that loses its check mark reports a change as well
      if (!e.GetParameter("checked", false))
        return;

      ezUInt32 uiSelection = 0;

      if (ezConversionUtils::StringToUInt(e.GetParameter("value", Rml::String()).c_str(), uiSelection).Succeeded() && uiSelection < EZ_ARRAY_SIZE(s_WindowModes))
      {
        m_PendingWindowDesc.m_WindowMode = s_WindowModes[uiSelection];

        // a fullscreen window uses the monitor's resolution
        RebuildResolutionList(e.GetCurrentElement()->GetOwnerDocument());
        UpdateResolutionEnabledState(e.GetCurrentElement()->GetOwnerDocument());
      }
      //
    });

  pContext->RegisterEventHandler("monitor-change", [this](Rml::Event& e)
    {
      // entry 0 is 'primary monitor' (-1), the rest map to the enumerated screens
      const ezInt32 iSelection = static_cast<Rml::ElementFormControlSelect*>(e.GetCurrentElement())->GetSelection();

      if (iSelection >= 0)
      {
        m_PendingWindowDesc.m_iMonitor = static_cast<ezInt8>(iSelection - 1);

        RebuildResolutionList(e.GetCurrentElement()->GetOwnerDocument());
      }
      //
    });

  pContext->RegisterEventHandler("resolution-change", [this](Rml::Event& e)
    {
      const ezInt32 iSelection = static_cast<Rml::ElementFormControlSelect*>(e.GetCurrentElement())->GetSelection();

      if (iSelection >= 0 && iSelection < (ezInt32)m_Resolutions.GetCount())
      {
        m_PendingWindowDesc.m_Resolution = m_Resolutions[iSelection];
      }
      //
    });

  pContext->RegisterEventHandler("display-apply", [this](Rml::Event& e)
    {
      // applied in TickMainThread(), resizing the window from a worker thread deadlocks
      m_bApplyDisplaySettings = true;
      //
    });
}

void ezRmlUiMainMenuComponent::Update()
{
  SyncSoundGroupVolumes();

  ezRmlUiCanvas2DComponent* pMenuCanvas = GetCanvas(m_hMenuCanvas);

  if (pMenuCanvas != nullptr && pMenuCanvas->IsActive())
  {
    if (ezRmlUiContext* pContext = pMenuCanvas->GetRmlContext())
    {
      if (Rml::ElementDocument* pDocument = pContext->GetDocument(0))
      {
        if (!m_bMenuHandlersRegistered)
        {
          m_bMenuHandlersRegistered = true;
          RegisterMenuEventHandlers(pContext);
          UpdateStats(pDocument);
        }

        if (m_bFocusPending)
        {
          m_bFocusPending = false;
          ezRmlUiUtils::FocusFirstElement(pDocument);
        }
      }
    }
  }

  ezRmlUiCanvas2DComponent* pSettingsCanvas = GetCanvas(m_hSettingsCanvas);

  if (pSettingsCanvas == nullptr)
    return;

  // done here, because this component updates before the canvas
  pSettingsCanvas->SetPassInput(!m_InputRebinding.IsBlockingUiInput());

  if (!pSettingsCanvas->IsActive())
    return;

  ezRmlUiContext* pContext = pSettingsCanvas->GetRmlContext();

  if (pContext == nullptr)
    return;

  // the canvas loads the document during its first update
  Rml::ElementDocument* pDocument = pContext->GetDocument(0);

  if (pDocument == nullptr)
    return;

  if (!m_bSettingsHandlersRegistered)
  {
    m_bSettingsHandlersRegistered = true;
    RegisterSettingsEventHandlers(pContext);
  }

  // these CVars can also be changed from elsewhere, e.g. the console
  ezRmlUiUtils::SetChecked(pDocument->GetElementById("check-vsync"), ezGameApplication::cvar_AppVSync);
  ezRmlUiUtils::SetChecked(pDocument->GetElementById("check-fps"), ezGameApplication::cvar_AppShowFPS);

  // The canvas only updates its document when it gets input. While a key is captured it gets none, so the changes from here have to be announced.
  bool bDocumentChanged = false;

  if (!m_bWidgetsInitialized)
  {
    m_bWidgetsInitialized = true;
    bDocumentChanged = true;

    if (auto pWindow = GetMainWindow())
    {
      m_PendingWindowDesc = pWindow->GetCreationDescription();
    }

    RebuildMonitorList(pDocument);
    RebuildResolutionList(pDocument);
    RebuildSoundGroupRows(pDocument);
    RebuildInputBindingRows(pDocument);
    InitializeWidgetsFromSettings(pDocument);
    UpdateValueLabels(pDocument);
    UpdateInputBindingLabels(pDocument);
  }

  if (m_bFocusPending)
  {
    m_bFocusPending = false;
    ezRmlUiUtils::FocusFirstElement(pDocument);
  }

  if (m_bUpdateValueLabels)
  {
    m_bUpdateValueLabels = false;
    bDocumentChanged = true;
    UpdateValueLabels(pDocument);
  }

  if (m_bUpdateBindingLabels)
  {
    m_bUpdateBindingLabels = false;
    bDocumentChanged = true;
    UpdateInputBindingLabels(pDocument);
  }

  ezRmlUiUtils::SetInnerRmlIfChanged(pDocument->GetElementById("display-status"), m_sDisplayStatus.GetData());

  if (auto pOverlay = pDocument->GetElementById("rebind-overlay"))
  {
    const bool bShowOverlay = m_InputRebinding.IsCapturing();

    if (pOverlay->IsClassSet("visible") != bShowOverlay)
    {
      pOverlay->SetClass("visible", bShowOverlay);
      bDocumentChanged = true;
    }
  }

  if (bDocumentChanged)
  {
    pSettingsCanvas->RequestUpdate();
  }
}

void ezRmlUiMainMenuComponent::TickMainThread()
{
  if (IsMenuOpen())
  {
    if (m_uiCursorOverrideId == 0)
    {
      // the game may hide the cursor or clip it to the window
      ezMouseCursorOverrideDesc cursorDesc;
      cursorDesc.m_OSCursor = ezMouseCursorOverride::ForceOSCursor;
      cursorDesc.m_bForceNoClip = true;

      m_uiCursorOverrideId = ezInputManager::PushMouseCursorOverride(cursorDesc);
    }
  }
  else
  {
    ezInputManager::PopMouseCursorOverride(m_uiCursorOverrideId);
    m_uiCursorOverrideId = 0;
  }

  if (m_InputRebinding.UpdateCapture())
  {
    m_bUpdateBindingLabels = true;
  }

  if (m_bApplySettings)
  {
    m_bApplySettings = false;
    ApplySettings();
  }

  if (m_bApplyDisplaySettings)
  {
    m_bApplyDisplaySettings = false;
    ApplyDisplaySettings();
  }
}

void ezRmlUiMainMenuComponent::InitializeWidgetsFromSettings(Rml::ElementDocument* pDocument)
{
  // Read from the 'Options.' CVars rather than from the engine state, so the menu shows the choice that was made,
  // even where several choices map to the same engine state.
  if (auto pSelect = ezRmlUiUtils::GetSelectElement(pDocument, "select-texture-filtering"))
  {
    pSelect->SetSelection(ezMath::Clamp<ezInt32>(cvar_OptionsTextureFiltering, 0, ezGALTextureQuality::Anisotropic16x));
  }

  if (auto pSelect = ezRmlUiUtils::GetSelectElement(pDocument, "select-texture-quality"))
  {
    pSelect->SetSelection(ezMath::Clamp<ezInt32>(cvar_OptionsTextureQuality, 0, EZ_ARRAY_SIZE(s_TextureQualityPresets) - 1));
  }

  if (auto pSelect = ezRmlUiUtils::GetSelectElement(pDocument, "select-shadow-quality"))
  {
    pSelect->SetSelection(ezMath::Clamp<ezInt32>(cvar_OptionsShadowQuality, 0, EZ_ARRAY_SIZE(s_ShadowQualityPresets) - 1));
  }

  ezStringBuilder sValue;

  {
    const bool bHasSound = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>() != nullptr;

    if (auto pInput = ezRmlUiUtils::GetInputElement(pDocument, "master-volume"))
    {
      ezRmlUiUtils::SetDisabled(pInput, !bHasSound);

      sValue.SetFormat("{}", (ezInt32)((float)cvar_OptionsMasterVolume * 100.0f));
      pInput->SetValue(sValue.GetData());
    }

    for (ezUInt32 i = 0; i < m_GroupVolumes.GetCount(); ++i)
    {
      sValue.SetFormat("sound-group-{}", i);

      auto pInput = ezRmlUiUtils::GetInputElement(pDocument, sValue.GetData());

      if (pInput == nullptr)
        continue;

      ezRmlUiUtils::SetDisabled(pInput, !bHasSound || m_SoundGroups[i].m_sSoundGroup.IsEmpty());

      sValue.SetFormat("{}", (ezInt32)(m_GroupVolumes[i] * 100.0f));
      pInput->SetValue(sValue.GetData());
    }

    if (auto pStatus = pDocument->GetElementById("audio-status"))
    {
      pStatus->SetInnerRML(bHasSound ? "" : "No sound plugin is loaded.");
    }
  }

  if (auto pInput = ezRmlUiUtils::GetInputElement(pDocument, "render-scale"))
  {
    sValue.SetFormat("{}", (ezInt32)ezMath::Round((float)ezGameApplication::cvar_AppRenderScale * 100.0f));
    pInput->SetValue(sValue.GetData());
  }

  if (auto pInput = ezRmlUiUtils::GetInputElement(pDocument, "ui-scale"))
  {
    sValue.SetFormat("{}", (ezInt32)((float)cvar_OptionsUiScale * 100.0f));
    pInput->SetValue(sValue.GetData());
  }

  {
    // a mode that isn't offered shows up as the closest match
    ezInt32 iSelected = ezWindowMode::IsFullscreen(m_PendingWindowDesc.m_WindowMode) ? 1 : 0;

    for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(s_WindowModes); ++i)
    {
      if (s_WindowModes[i] == m_PendingWindowDesc.m_WindowMode)
      {
        iSelected = i;
        break;
      }
    }

    // this ends up in the 'window-mode-change' handler, which sets the same mode again
    for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(s_WindowModes); ++i)
    {
      sValue.SetFormat("window-mode-{}", i);
      ezRmlUiUtils::SetChecked(ezRmlUiUtils::GetInputElement(pDocument, sValue.GetData()), i == iSelected);
    }
  }

  if (auto pInput = ezRmlUiUtils::GetInputElement(pDocument, "player-name"))
  {
    pInput->SetValue(cvar_OptionsPlayerName.GetValue().GetData());
  }

  UpdateResolutionEnabledState(pDocument);
}

void ezRmlUiMainMenuComponent::LoadSoundGroupVolumes()
{
  m_GroupVolumes.SetCount(m_SoundGroups.GetCount(), 1.0f);

  ezHybridArray<ezStringView, 8> pairs;
  ezStringView(cvar_OptionsSoundGroupVolumes.GetValue().GetData()).Split(false, pairs, ";");

  for (ezStringView sPair : pairs)
  {
    const char* szSeparator = sPair.FindSubString("=");

    if (szSeparator == nullptr)
      continue;

    const ezStringView sGroup(sPair.GetStartPointer(), szSeparator);
    const ezStringView sVolume(szSeparator + 1, sPair.GetEndPointer());

    for (ezUInt32 i = 0; i < m_SoundGroups.GetCount(); ++i)
    {
      if (m_SoundGroups[i].m_sSoundGroup == sGroup)
      {
        double fVolume = 1.0;
        if (ezConversionUtils::StringToFloat(sVolume, fVolume).Succeeded())
        {
          m_GroupVolumes[i] = ezMath::Clamp((float)fVolume, 0.0f, 1.0f);
        }
        break;
      }
    }
  }
}

void ezRmlUiMainMenuComponent::SyncSoundGroupVolumes()
{
  if (m_GroupVolumes.GetCount() == m_SoundGroups.GetCount())
    return;

  LoadSoundGroupVolumes();

  // the rows have to be rebuilt as well
  m_bWidgetsInitialized = false;
}

void ezRmlUiMainMenuComponent::SaveSoundGroupVolumes() const
{
  ezStringBuilder sVolumes;

  for (ezUInt32 i = 0; i < m_SoundGroups.GetCount(); ++i)
  {
    if (m_SoundGroups[i].m_sSoundGroup.IsEmpty())
      continue;

    sVolumes.AppendFormat("{}{}={}", sVolumes.IsEmpty() ? "" : ";", m_SoundGroups[i].m_sSoundGroup, m_GroupVolumes[i]);
  }

  cvar_OptionsSoundGroupVolumes = sVolumes.GetData();
}

void ezRmlUiMainMenuComponent::RebuildSoundGroupRows(Rml::ElementDocument* pDocument)
{
  auto pRows = pDocument->GetElementById("sound-group-rows");

  if (pRows == nullptr)
    return;

  ezStringBuilder sRml, sRow;

  for (ezUInt32 i = 0; i < m_SoundGroups.GetCount(); ++i)
  {
    sRow.SetFormat("<tr><td>{}</td><td><input type=\"range\" min=\"0\" max=\"100\" value=\"{}\" id=\"sound-group-{}\" onchange=\"sound-group-volume-change\"/></td><td class=\"value\" id=\"sound-group-{}-value\"></td></tr>",
      m_SoundGroups[i].m_sLabel, (ezInt32)(m_GroupVolumes[i] * 100.0f), i, i);

    sRml.Append(sRow.GetView());
  }

  pRows->SetInnerRML(sRml.GetData());
}

void ezRmlUiMainMenuComponent::UpdateValueLabels(Rml::ElementDocument* pDocument)
{
  // Must not be called from a slider's change event: writing to the same row makes the slider send the event again, endlessly.
  ezStringBuilder sId, sValue;

  auto SetLabel = [&](const char* szId, const char* szText)
  {
    if (auto pElement = pDocument->GetElementById(szId))
    {
      pElement->SetInnerRML(szText);
    }
  };

  sValue.SetFormat("{}%%", (ezInt32)((float)cvar_OptionsMasterVolume * 100.0f));
  SetLabel("master-volume-value", sValue.GetData());

  sValue.SetFormat("{}%%", (ezInt32)((float)cvar_OptionsUiScale * 100.0f));
  SetLabel("ui-scale-value", sValue.GetData());

  sValue.SetFormat("{}%%", (ezInt32)ezMath::Round((float)ezGameApplication::cvar_AppRenderScale * 100.0f));
  SetLabel("render-scale-value", sValue.GetData());

  for (ezUInt32 i = 0; i < m_GroupVolumes.GetCount(); ++i)
  {
    sId.SetFormat("sound-group-{}-value", i);
    sValue.SetFormat("{}%%", (ezInt32)(m_GroupVolumes[i] * 100.0f));

    SetLabel(sId.GetData(), sValue.GetData());
  }
}

void ezRmlUiMainMenuComponent::RebuildInputBindingRows(Rml::ElementDocument* pDocument)
{
  auto pRows = pDocument->GetElementById("input-binding-rows");
  const auto bindings = m_InputRebinding.GetBindings();

  if (pRows == nullptr || bindings.IsEmpty())
    return;

  // with several input sets the same action name can appear more than once
  bool bShowInputSet = false;

  for (const auto& binding : bindings)
  {
    if (binding.m_sInputSet != bindings[0].m_sInputSet)
    {
      bShowInputSet = true;
      break;
    }
  }

  ezStringBuilder sRml, sRow, sLabel;

  for (ezUInt32 i = 0; i < bindings.GetCount(); ++i)
  {
    const auto& binding = bindings[i];

    sLabel = ezInputManager::GetActionDisplayName(binding.m_sAction);

    if (bShowInputSet)
    {
      sLabel.PrependFormat("{}: ", binding.m_sInputSet);
    }

    sRow.SetFormat("<tr><td>{}</td>", sLabel);

    // the RML document has one column per slot
    for (ezUInt32 uiSlot = 0; uiSlot < ezInputRebinding::MaxSlots; ++uiSlot)
    {
      // the caption is set by UpdateInputBindingLabels()
      sRow.AppendFormat("<td><button class=\"binding\" id=\"input-bind-{}-{}\" onclick=\"input-rebind\"></button></td>", i, uiSlot);
    }

    sRow.AppendFormat("<td><button class=\"restore\" id=\"input-restore-{}\" onclick=\"input-restore\">&lt;</button></td>", i);
    sRow.AppendFormat("<td><button class=\"unbind\" id=\"input-unbind-{}\" onclick=\"input-unbind\">X</button></td>", i);

    // the empty margin column
    sRow.Append("<td></td></tr>");

    sRml.Append(sRow.GetView());
  }

  pRows->SetInnerRML(sRml.GetData());
}

void ezRmlUiMainMenuComponent::UpdateInputBindingLabels(Rml::ElementDocument* pDocument)
{
  ezStringBuilder sId, sLabel;

  for (ezUInt32 i = 0; i < m_InputRebinding.GetBindings().GetCount(); ++i)
  {
    for (ezUInt32 uiSlot = 0; uiSlot < ezInputRebinding::MaxSlots; ++uiSlot)
    {
      sId.SetFormat("input-bind-{}-{}", i, uiSlot);

      auto pButton = pDocument->GetElementById(sId.GetData());

      if (pButton == nullptr)
        continue;

      const ezString sSlot = m_InputRebinding.GetBoundSlot(i, uiSlot);

      if (m_InputRebinding.IsCapturing(i, uiSlot))
      {
        sLabel = "Press a key...";
      }
      else if (sSlot.IsEmpty())
      {
        sLabel = "-";
      }
      else
      {
        sLabel = ezInputManager::GetInputSlotDisplayName(sSlot);
      }

      pButton->SetInnerRML(sLabel.GetData());
    }
  }
}

void ezRmlUiMainMenuComponent::RestoreDefaultSettings(Rml::ElementDocument* pDocument)
{
  // Only the visible settings are reset. Checking visibility instead of a tab index keeps this working with a different tab layout.
  // The display settings are left alone, they only change through 'Apply'.
  auto IsShown = [&](const char* szId) -> bool
  {
    auto pElement = pDocument->GetElementById(szId);
    return pElement != nullptr && pElement->IsVisible(true);
  };

  if (IsShown("check-fps"))
  {
    ezGameApplication::cvar_AppShowFPS = ezGameApplication::cvar_AppShowFPS.GetValue(ezCVarValue::Default);
  }

  if (IsShown("player-name"))
  {
    cvar_OptionsPlayerName = cvar_OptionsPlayerName.GetValue(ezCVarValue::Default);
  }

  if (IsShown("ui-scale"))
  {
    cvar_OptionsUiScale = cvar_OptionsUiScale.GetValue(ezCVarValue::Default);
  }

  if (IsShown("check-vsync"))
  {
    ezGameApplication::cvar_AppVSync = ezGameApplication::cvar_AppVSync.GetValue(ezCVarValue::Default);
  }

  if (IsShown("render-scale"))
  {
    ezGameApplication::cvar_AppRenderScale = ezGameApplication::cvar_AppRenderScale.GetValue(ezCVarValue::Default);
  }

  if (IsShown("select-texture-filtering"))
  {
    cvar_OptionsTextureFiltering = cvar_OptionsTextureFiltering.GetValue(ezCVarValue::Default);
  }

  if (IsShown("select-texture-quality"))
  {
    cvar_OptionsTextureQuality = cvar_OptionsTextureQuality.GetValue(ezCVarValue::Default);
  }

  if (IsShown("select-shadow-quality"))
  {
    cvar_OptionsShadowQuality = cvar_OptionsShadowQuality.GetValue(ezCVarValue::Default);
  }

  if (IsShown("master-volume"))
  {
    cvar_OptionsMasterVolume = cvar_OptionsMasterVolume.GetValue(ezCVarValue::Default);
  }

  ezStringBuilder sId;
  for (ezUInt32 i = 0; i < m_GroupVolumes.GetCount(); ++i)
  {
    sId.SetFormat("sound-group-{}", i);

    if (IsShown(sId))
    {
      m_GroupVolumes[i] = 1.0f;
    }
  }

  if (IsShown("input-binding-rows"))
  {
    m_InputRebinding.RestoreDefaults();
  }

  SaveSoundGroupVolumes();
  RequestApplySettings();

  InitializeWidgetsFromSettings(pDocument);
  UpdateValueLabels(pDocument);
  UpdateInputBindingLabels(pDocument);
}

void ezRmlUiMainMenuComponent::UpdateStats(Rml::ElementDocument* pDocument)
{
  auto pElement = pDocument->GetElementById("menu-stats");

  if (pElement == nullptr)
    return;

  // __DATE__ is the compile date of this file, the closest to a build date that is available
  ezStringBuilder sStats;
  sStats.SetFormat("ezEngine {}.{}.{} ({})<br/>{}<br/>{} {}<br/>Built {}",
    BUILDSYSTEM_SDKVERSION_MAJOR, BUILDSYSTEM_SDKVERSION_MINOR, BUILDSYSTEM_SDKVERSION_PATCH, BUILDSYSTEM_BUILDTYPE,
    ezGameApplicationBase::GetGameApplicationBaseInstance()->GetApplicationName(),
    ezSystemInformation::Get().GetPlatformName(), ezSystemInformation::Get().Is64BitOS() ? "64 Bit" : "32 Bit",
    __DATE__);

  pElement->SetInnerRML(sStats.GetData());
}

void ezRmlUiMainMenuComponent::RebuildMonitorList(Rml::ElementDocument* pDocument)
{
  m_Screens.Clear();
  ezScreen::EnumerateScreens(m_Screens).IgnoreResult();

  auto pSelect = ezRmlUiUtils::GetSelectElement(pDocument, "select-monitor");

  if (pSelect == nullptr)
    return;

  pSelect->RemoveAll();

  // index 0 maps to monitor -1
  ezRmlUiUtils::AddIndexedOption(pSelect, "Primary Monitor", 0);

  ezStringBuilder sName;

  for (ezUInt32 i = 0; i < m_Screens.GetCount(); ++i)
  {
    const auto& screen = m_Screens[i];

    sName = screen.m_sDisplayName.IsEmpty() ? screen.m_sDisplayID : screen.m_sDisplayName;

    if (screen.m_bIsPrimary)
    {
      sName.Append(" (Primary)");
    }

    ezRmlUiUtils::AddIndexedOption(pSelect, sName.GetData(), i + 1);
  }

  if (m_PendingWindowDesc.m_iMonitor >= 0 && m_PendingWindowDesc.m_iMonitor < (ezInt8)m_Screens.GetCount())
  {
    pSelect->SetSelection(m_PendingWindowDesc.m_iMonitor + 1);
  }
  else
  {
    pSelect->SetSelection(0);
  }
}

void ezRmlUiMainMenuComponent::RebuildResolutionList(Rml::ElementDocument* pDocument)
{
  m_Resolutions.Clear();

  ezInt32 iScreen = m_PendingWindowDesc.m_iMonitor;

  if (iScreen < 0)
  {
    for (ezUInt32 i = 0; i < m_Screens.GetCount(); ++i)
    {
      if (m_Screens[i].m_bIsPrimary)
      {
        iScreen = i;
        break;
      }
    }
  }

  // A fullscreen window shows the monitor's resolution. m_PendingWindowDesc keeps the windowed resolution,
  // so that switching back to windowed returns to the user's size.
  ezSizeU32 shownResolution = m_PendingWindowDesc.m_Resolution;

  if (ezWindowMode::IsFullscreen(m_PendingWindowDesc.m_WindowMode) && iScreen >= 0 && iScreen < (ezInt32)m_Screens.GetCount())
  {
    shownResolution.width = m_Screens[iScreen].m_iResolutionX;
    shownResolution.height = m_Screens[iScreen].m_iResolutionY;
  }

  if (iScreen >= 0 && iScreen < (ezInt32)m_Screens.GetCount())
  {
    // each resolution is listed once per bit depth and refresh rate
    for (const auto& res : m_Screens[iScreen].m_SupportedResolutions)
    {
      const ezSizeU32 size(res.m_uiResolutionX, res.m_uiResolutionY);

      if (!m_Resolutions.Contains(size))
      {
        m_Resolutions.PushBack(size);
      }
    }
  }

  if (m_Resolutions.IsEmpty())
  {
    for (const auto& size : s_FallbackResolutions)
    {
      m_Resolutions.PushBack(size);
    }
  }

  m_Resolutions.Sort([](const ezSizeU32& lhs, const ezSizeU32& rhs)
    { return (lhs.width * lhs.height) > (rhs.width * rhs.height); });

  auto pSelect = ezRmlUiUtils::GetSelectElement(pDocument, "select-resolution");

  if (pSelect == nullptr)
    return;

  pSelect->RemoveAll();

  ezInt32 iCurrent = -1;
  ezStringBuilder sLabel;

  for (ezUInt32 i = 0; i < m_Resolutions.GetCount(); ++i)
  {
    sLabel.SetFormat("{} x {}", m_Resolutions[i].width, m_Resolutions[i].height);
    ezRmlUiUtils::AddIndexedOption(pSelect, sLabel.GetData(), i);

    if (m_Resolutions[i] == shownResolution)
    {
      iCurrent = i;
    }
  }

  if (iCurrent < 0)
  {
    // e.g. a freely resized window, added as an extra entry rather than silently changed
    sLabel.SetFormat("{} x {}", shownResolution.width, shownResolution.height);
    m_Resolutions.PushBack(shownResolution);
    iCurrent = m_Resolutions.GetCount() - 1;

    ezRmlUiUtils::AddIndexedOption(pSelect, sLabel.GetData(), iCurrent);
  }

  pSelect->SetSelection(iCurrent);
}

void ezRmlUiMainMenuComponent::UpdateResolutionEnabledState(Rml::ElementDocument* pDocument)
{
  if (auto pSelect = ezRmlUiUtils::GetSelectElement(pDocument, "select-resolution"))
  {
    ezRmlUiUtils::SetDisabled(pSelect, ezWindowMode::IsFullscreen(m_PendingWindowDesc.m_WindowMode));
  }
}

void ezRmlUiMainMenuComponent::ApplyDisplaySettings()
{
  auto pWindow = GetMainWindow();

  if (pWindow == nullptr)
  {
    m_sDisplayStatus = "No window to configure.";
    return;
  }

  // ezGameState prefers this file over the project's
  if (m_PendingWindowDesc.SaveToDDL(ezGameState::s_sUserWindowConfigFile).Failed())
  {
    m_sDisplayStatus = "Failed to save the window configuration.";
    return;
  }

  if (pWindow->Reconfigure(m_PendingWindowDesc).Succeeded())
  {
    m_sDisplayStatus = "Applied.";
  }
  else
  {
    m_sDisplayStatus = "Saved. Takes effect after a restart.";
  }
}

EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiMainMenuComponent);
