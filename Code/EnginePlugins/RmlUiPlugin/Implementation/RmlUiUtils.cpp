#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/RmlUiUtils.h>

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <RmlUi/Core/Event.h>

namespace ezRmlUiUtils
{
  Rml::ElementFormControlSelect* GetSelectElement(Rml::ElementDocument* pDocument, const char* szId)
  {
    if (pDocument == nullptr)
      return nullptr;

    return rmlui_dynamic_cast<Rml::ElementFormControlSelect*>(pDocument->GetElementById(szId));
  }

  Rml::ElementFormControlInput* GetInputElement(Rml::ElementDocument* pDocument, const char* szId)
  {
    if (pDocument == nullptr)
      return nullptr;

    return rmlui_dynamic_cast<Rml::ElementFormControlInput*>(pDocument->GetElementById(szId));
  }

  void AddIndexedOption(Rml::ElementFormControlSelect* pSelect, ezStringView sLabel, ezUInt32 uiIndex)
  {
    if (pSelect == nullptr)
      return;

    ezStringBuilder sValue;
    sValue.SetFormat("{}", uiIndex);

    ezStringBuilder sLabelStr = sLabel;
    pSelect->Add(sLabelStr.GetData(), sValue.GetData());
  }

  float GetChangedValue(Rml::Event& ref_event, float fDefault)
  {
    return ref_event.GetParameter("value", fDefault);
  }

  void SetChecked(Rml::Element* pElement, bool bChecked)
  {
    if (pElement == nullptr || pElement->HasAttribute("checked") == bChecked)
      return;

    if (bChecked)
    {
      pElement->SetAttribute("checked", "");
    }
    else
    {
      pElement->RemoveAttribute("checked");
    }
  }

  void SetDisabled(Rml::Element* pElement, bool bDisabled)
  {
    if (pElement == nullptr || pElement->HasAttribute("disabled") == bDisabled)
      return;

    if (bDisabled)
    {
      pElement->SetAttribute("disabled", "");
    }
    else
    {
      pElement->RemoveAttribute("disabled");
    }
  }

  void SetInnerRmlIfChanged(Rml::Element* pElement, const char* szRml)
  {
    if (pElement == nullptr || pElement->GetInnerRML() == szRml)
      return;

    pElement->SetInnerRML(szRml);
  }

  void FocusFirstElement(Rml::ElementDocument* pDocument, const char* szSelector)
  {
    if (pDocument == nullptr)
      return;

    if (Rml::Element* pElement = pDocument->QuerySelector(szSelector))
    {
      pElement->Focus(true);
    }
  }
} // namespace ezRmlUiUtils

EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Implementation_RmlUiUtils);
