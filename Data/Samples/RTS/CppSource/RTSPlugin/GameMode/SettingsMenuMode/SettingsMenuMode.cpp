#include <RTSPlugin/RTSPluginPCH.h>

#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/System/Screen.h>
#include <RTSPlugin/GameMode/SettingsMenuMode/SettingsMenuMode.h>
#include <RTSPlugin/GameState/RTSGameState.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>

RtsSettingsMenuMode::RtsSettingsMenuMode() = default;
RtsSettingsMenuMode::~RtsSettingsMenuMode() = default;

void RtsSettingsMenuMode::OnActivateMode()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  SetUiActive(m_pMainWorld, ezTempHashedString("app-settings"), true);

  ezGameObject* pUIObject = nullptr;
  if (m_pMainWorld->TryGetObjectWithGlobalKey(ezTempHashedString("app-settings"), pUIObject))
  {
    ezRmlUiCanvas2DComponent* pUiComponent = nullptr;
    if (pUIObject->TryGetComponentOfBaseType(pUiComponent))
    {
      m_hSettingsMenu = pUiComponent->GetHandle();

      ezRmlUiContext* pRmlContext = pUiComponent->GetOrCreateRmlContext();

      pRmlContext->RegisterEventHandler("toggle-vsync", [this](Rml::Event& e)
        {
          ezGameApplication::cvar_AppVSync = e.GetCurrentElement()->HasAttribute("checked");
          //
        });

      pRmlContext->RegisterEventHandler("toggle-fps", [this](Rml::Event& e)
        {
          ezGameApplication::cvar_AppShowFPS = e.GetCurrentElement()->HasAttribute("checked");
          //
        });

      pRmlContext->RegisterEventHandler("ui-scale-change", [this](Rml::Event& e)
        {
          if (auto pRangeControl = static_cast<Rml::ElementFormControlInput*>(e.GetCurrentElement()))
          {
            if (Rml::Variant* valueVariant = pRangeControl->GetAttribute("value"))
            {
              // Convert to integer
              int value = valueVariant->Get<int>();
              m_pGameState->m_fUiScale = value / 100.0f;
            }
          }
          //
        });

      pRmlContext->RegisterEventHandler("example-button", [this](Rml::Event& e)
        {
          m_uiButtonClickCount++;
          //
        });

      pRmlContext->RegisterEventHandler("example-checkbox-change", [this](Rml::Event& e)
        {
          // The checkbox state is automatically updated by RmlUi
          // We don't need to track it separately since we can read it from the element
          //
        });

      pRmlContext->RegisterEventHandler("example-text-change", [this](Rml::Event& e)
        {
          // Text input value is automatically updated by RmlUi
          //
        });

      pRmlContext->RegisterEventHandler("example-radio-change", [this](Rml::Event& e)
        {
          // Radio button state is automatically updated by RmlUi
          //
        });

      pRmlContext->RegisterEventHandler("example-range-change", [this](Rml::Event& e)
        {
          // Range slider value is automatically updated by RmlUi
          //
        });

      pRmlContext->RegisterEventHandler("example-textarea-change", [this](Rml::Event& e)
        {
          // Textarea value is automatically updated by RmlUi
          //
        });

      pRmlContext->RegisterEventHandler("example-select-change", [this](Rml::Event& e)
        {
          // Select value is automatically updated by RmlUi
          //
        });
    }
  }
}

void RtsSettingsMenuMode::OnDeactivateMode()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  SetUiActive(m_pMainWorld, ezTempHashedString("app-settings"), false);
}

void RtsSettingsMenuMode::OnBeforeWorldUpdate()
{
  ezRmlUiCanvas2DComponent* pUiComponent = nullptr;
  if (m_pMainWorld->TryGetComponent(m_hSettingsMenu, pUiComponent))
  {
    if (auto pActiveModeElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("check-vsync"))
    {
      if (ezGameApplication::cvar_AppVSync)
      {
        pActiveModeElement->SetAttribute("checked", "");
      }
      else
      {
        pActiveModeElement->RemoveAttribute("checked");
      }
    }

    if (auto pActiveModeElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("check-fps"))
    {
      if (ezGameApplication::cvar_AppShowFPS)
      {
        pActiveModeElement->SetAttribute("checked", "");
      }
      else
      {
        pActiveModeElement->RemoveAttribute("checked");
      }
    }

    if (auto pCounterElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("button-counter"))
    {
      ezStringBuilder sCounterText;
      sCounterText.SetFormat("Clicks: {}", m_uiButtonClickCount);
      pCounterElement->SetInnerRML(sCounterText.GetData());
    }

    if (auto pCheckboxStatusElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("checkbox-status"))
    {
      if (auto pCheckboxElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("example-checkbox"))
      {
        const char* szStatus = pCheckboxElement->HasAttribute("checked") ? "checked" : "unchecked";
        pCheckboxStatusElement->SetInnerRML(szStatus);
      }
    }

    if (auto pTextMirrorElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("text-input-mirror"))
    {
      if (auto pTextInputElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("example-text-input"))
      {
        // Cast to the specific input element type to access GetValue()
        if (auto pInputControl = static_cast<Rml::ElementFormControlInput*>(pTextInputElement))
        {
          Rml::String sValue = pInputControl->GetValue();
          pTextMirrorElement->SetInnerRML(sValue);
        }
      }
    }

    if (auto pRadioStatusElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("radio-status"))
    {
      const char* szStatus = "none selected";

      if (auto pRadio1 = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("radio-1"))
      {
        if (pRadio1->HasAttribute("checked"))
        {
          szStatus = "Option 1";
        }
      }

      if (auto pRadio2 = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("radio-2"))
      {
        if (pRadio2->HasAttribute("checked"))
        {
          szStatus = "Option 2";
        }
      }

      pRadioStatusElement->SetInnerRML(szStatus);
    }

    if (auto pRangeValueElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("range-value"))
    {
      if (auto pRangeElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("example-range"))
      {
        if (auto pRangeControl = static_cast<Rml::ElementFormControlInput*>(pRangeElement))
        {
          Rml::String sValue = pRangeControl->GetValue();
          pRangeValueElement->SetInnerRML(sValue);
        }
      }
    }

    if (auto pTextareaMirrorElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("textarea-mirror"))
    {
      if (auto pTextareaElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("example-textarea"))
      {
        if (auto pTextareaControl = static_cast<Rml::ElementFormControlTextArea*>(pTextareaElement))
        {
          Rml::String sValue = pTextareaControl->GetValue();
          pTextareaMirrorElement->SetInnerRML(sValue);
        }
      }
    }

    if (auto pSelectValueElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("select-value"))
    {
      if (auto pSelectElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById("example-select"))
      {
        if (auto pSelectControl = static_cast<Rml::ElementFormControlSelect*>(pSelectElement))
        {
          int iSelection = pSelectControl->GetSelection();
          if (iSelection >= 0)
          {
            Rml::String sValue = pSelectControl->GetOption(iSelection)->GetInnerRML();
            pSelectValueElement->SetInnerRML(sValue);
          }
        }
      }
    }
  }
}

void RtsSettingsMenuMode::OnProcessInput(const RtsMouseInputState& MouseInput, bool bUiWantsInput)
{
  if (ezInputManager::GetInputSlotState(ezInputSlot_KeyEscape) == ezKeyState::Pressed)
  {
    m_pGameState->SwitchToGameMode(m_pGameState->GetPrevGameMode());
  }
}
