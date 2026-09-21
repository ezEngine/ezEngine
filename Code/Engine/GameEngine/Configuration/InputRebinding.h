#pragma once

#include <Core/Input/InputManager.h>
#include <GameEngine/GameEngineDLL.h>

/// Lets the player change which keys and buttons trigger input actions, e.g. from a 'Controls' page in a settings menu.
///
/// Contains no UI. Changes are applied to ezInputManager right away, but only written to disk by SaveUserBindings().
/// ezGameApplication applies the saved bindings on the next start.
///
/// Typical use:
///   * CollectActions() once the game state has registered its input actions, followed by ApplyUserBindings().
///   * UpdateCapture() every frame on the main thread, StartCapture() when the player clicks a binding.
///   * SetBinding() / ClearBindings() / RestoreDefaults() for other buttons of the UI.
///   * SaveUserBindings() if HasUnsavedChanges().
///
/// Rebinding a slot removes it from all other actions of the same input set, so that one key never triggers two actions of one set.
class EZ_GAMEENGINE_DLL ezInputRebinding
{
public:
  static constexpr ezUInt32 MaxSlots = ezInputActionConfig::MaxInputSlotAlternatives;

  struct Binding
  {
    ezString m_sInputSet;
    ezString m_sAction;

    /// What RestoreDefault() and RestoreDefaults() return to.
    ///
    /// Taken from the project's input configuration file (ezGameAppInputConfig::s_sConfigFile). Actions that are registered in code
    /// aren't in that file, for those it is the configuration when CollectActions() was called.
    ezInputActionConfig m_DefaultConfig;
  };

  /// Collects all actions of the given input sets and remembers their current configuration as the defaults.
  ///
  /// When inputSets is empty, all input sets are used except the ones that the engine registers for itself (see IsEngineInputSet()).
  /// Must be called before ApplyUserBindings(), otherwise those user bindings become the defaults.
  void CollectActions(ezArrayPtr<const ezString> inputSets);

  /// Applies ezGameAppInputConfig::s_sUserConfigFile.
  ///
  /// ezGameApplication already does this at startup, but a game state that registers its actions in code afterwards
  /// overwrites the player's bindings. Call this once those actions are registered.
  static void ApplyUserBindings();

  /// Whether the input set is one that ezGameApplication or the console registers for developer shortcuts.
  static bool IsEngineInputSet(ezStringView sInputSet);

  ezArrayPtr<const Binding> GetBindings() const { return m_Bindings; }

  /// Returns the input slot that currently triggers the action in the given alternative, or an empty string (ezInputSlot_None).
  ///
  /// Use ezInputManager::GetInputSlotDisplayName() to get a name to show to the player.
  ezString GetBoundSlot(ezUInt32 uiBinding, ezUInt32 uiSlot) const;

  /// Binds the input slot to the action and applies it.
  ///
  /// An empty sInputSlot removes the binding. If the action already uses the input slot in another alternative, it is moved.
  /// Out of range indices are ignored.
  void SetBinding(ezUInt32 uiBinding, ezUInt32 uiSlot, ezStringView sInputSlot);

  /// Removes all input slots from the action.
  void ClearBindings(ezUInt32 uiBinding);

  /// Returns the action to its default configuration, see Binding::m_DefaultConfig.
  ///
  /// The default input slots are removed from all other actions of the same input set.
  void RestoreDefault(ezUInt32 uiBinding);

  /// Returns all collected actions to their default configuration, see Binding::m_DefaultConfig.
  ///
  /// Unless the bindings are changed again before the next SaveUserBindings(), that call deletes ezGameAppInputConfig::s_sUserConfigFile
  /// instead of writing it, so that later changes to the project's configuration reach the player.
  void RestoreDefaults();

  /// Whether SetBinding(), ClearBindings() or RestoreDefaults() changed anything since the last SaveUserBindings() or CollectActions().
  bool HasUnsavedChanges() const { return m_bUnsavedChanges; }

  /// Writes all collected actions to ezGameAppInputConfig::s_sUserConfigFile, or deletes the file after RestoreDefaults().
  ///
  /// All actions are written, since rebinding one action can remove an input slot from another.
  /// Writes the file even without unsaved changes, check HasUnsavedChanges() to avoid that.
  void SaveUserBindings();

  /// Makes UpdateCapture() bind the next key or button that is pressed.
  void StartCapture(ezUInt32 uiBinding, ezUInt32 uiSlot);
  void CancelCapture();

  bool IsCapturing() const { return m_iCaptureBinding >= 0; }
  bool IsCapturing(ezUInt32 uiBinding, ezUInt32 uiSlot) const { return m_iCaptureBinding == (ezInt32)uiBinding && m_iCaptureSlot == (ezInt32)uiSlot; }

  /// Whether the UI should currently ignore all input.
  ///
  /// True while capturing and until the input that ended the capture is released, otherwise e.g. the click that
  /// gets bound would also activate the button under the cursor.
  bool IsBlockingUiInput() const { return IsCapturing() || !m_sReleasePending.IsEmpty(); }

  /// Binds the first key or button that is pressed after StartCapture(). ESC cancels the capture instead.
  ///
  /// Only starts looking once all input that was held at StartCapture() (e.g. the click on the UI button) is released.
  /// Axes (mouse movement, analog sticks) are never 'pressed', those can only be bound through SetBinding().
  ///
  /// Must be called every frame from the main thread, also when not capturing, see IsBlockingUiInput().
  /// Returns true when a capture ended, through binding something or through ESC.
  ///
  /// When the UI also closes on ESC, check IsCapturing() before this call, so that the ESC that cancels the capture doesn't close the UI as well.
  bool UpdateCapture();

private:
  ezDynamicArray<Binding> m_Bindings;

  /// Which binding and slot is waiting for input, -1 while none is.
  ezInt32 m_iCaptureBinding = -1;
  ezInt32 m_iCaptureSlot = -1;

  /// Whether everything was released since the capture started.
  bool m_bStartInputReleased = false;

  /// The input slot that ended the capture, until it is released.
  ezString m_sReleasePending;

  bool m_bUnsavedChanges = false;

  /// Set by RestoreDefaults(), makes SaveUserBindings() delete the file.
  bool m_bDefaultsRestored = false;
};
