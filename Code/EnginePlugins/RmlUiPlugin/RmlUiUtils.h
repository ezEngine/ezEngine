#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Strings/StringView.h>

namespace Rml
{
  class Element;
  class ElementDocument;
  class ElementFormControlInput;
  class ElementFormControlSelect;
  class Event;
} // namespace Rml

/// Helpers for filling and reading RmlUi form controls from code.
///
/// All functions accept nullptr elements and do nothing in that case, so that a document may leave out some of the elements
/// that the code expects.
namespace ezRmlUiUtils
{
  /// Returns nullptr if there is no element with that ID or it isn't a <select>.
  EZ_RMLUIPLUGIN_DLL Rml::ElementFormControlSelect* GetSelectElement(Rml::ElementDocument* pDocument, const char* szId);

  /// Returns nullptr if there is no element with that ID or it isn't an <input>.
  EZ_RMLUIPLUGIN_DLL Rml::ElementFormControlInput* GetInputElement(Rml::ElementDocument* pDocument, const char* szId);

  /// Adds an option to a <select> whose value is the index. Use GetSelection() to read the index back.
  EZ_RMLUIPLUGIN_DLL void AddIndexedOption(Rml::ElementFormControlSelect* pSelect, ezStringView sLabel, ezUInt32 uiIndex);

  /// Returns the new value from a 'change' event of a range <input> (a slider).
  ///
  /// RmlUi sends the event before it writes the value to the element, so reading the element in the event handler returns the old value.
  EZ_RMLUIPLUGIN_DLL float GetChangedValue(Rml::Event& ref_event, float fDefault = 0.0f);

  /// Checks or unchecks a checkbox or radio button.
  ///
  /// Writing the 'checked' attribute always sends a 'change' event, even if the state stays the same, so it is only written when it differs.
  /// That makes it safe to call every frame.
  EZ_RMLUIPLUGIN_DLL void SetChecked(Rml::Element* pElement, bool bChecked);

  /// Disables or enables an element.
  ///
  /// Writing the 'disabled' attribute always sends an event, even if the state stays the same, so it is only written when it differs.
  /// That makes it safe to call every frame.
  EZ_RMLUIPLUGIN_DLL void SetDisabled(Rml::Element* pElement, bool bDisabled);

  /// Replaces the element's content, but only if it differs.
  ///
  /// Setting the content recreates the element's children, which is wasteful every frame and resets the state of interactive children.
  EZ_RMLUIPLUGIN_DLL void SetInnerRmlIfChanged(Rml::Element* pElement, const char* szRml);

  /// Focuses the first element that matches the CSS selector.
  ///
  /// Without a focused element keyboard navigation has nothing to start from, so this should be done whenever a page is opened.
  EZ_RMLUIPLUGIN_DLL void FocusFirstElement(Rml::ElementDocument* pDocument, const char* szSelector = "button");
} // namespace ezRmlUiUtils
