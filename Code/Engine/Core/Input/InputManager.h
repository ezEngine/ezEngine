#pragma once

#include <Core/Input/InputDevice.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Communication/Event.h>

/// \brief A struct that defines how to register an input action.
struct EZ_CORE_DLL ezInputActionConfig
{
  /// \brief Change this value to adjust how many input slots may trigger the same action.
  enum { MaxInputSlotAlternatives = 3 };

  ezInputActionConfig();

  /// \brief If this is set to true, the value of the action is scaled by the time difference since the last input update. Default is true.
  ///
  /// You should enable this, if the value of the triggered action is used to modify how much to e.g. move an or rotate something.
  /// For example, if an action 'RotateLeft' will rotate the player to the left, then he should rotate each frame an amount that is dependent on how
  /// much time has passed since the last update and by how much the button is pressed (e.g. a thumb-stick can be pressed only slightly).
  /// Mouse input, however, should not get scaled, because when the user moved the mouse one centimeter in the last frame, then that is an absolute
  /// movement and it does not depend on how much time elapsed. Therefore the ezInputManager will take care NOT to scale input from such devices,
  /// whereas input from a thumb-stick or a keyboard key would be scaled by the elapsed time.
  ///
  /// However, if you for example use a thumb-stick to position something on screen (e.g. a cursor), then that action should NEVER be scaled by the
  /// elapsed time, because you are actually interested in the absolute value of the thumb-stick (and thus the action). In such cases you should
  /// disable time scaling.
  ///
  /// When you have an action where your are not interested in the value, only in whether it is triggered, at all, you can ignore time scaling altogether.
  bool m_bApplyTimeScaling;

  /// \brief Which input slots will trigger this action.
  ezString m_sInputSlotTrigger[MaxInputSlotAlternatives];

  /// \brief This scale is applied to the input slot value (before time scaling). Positive values mean a linear scaling, negative values an exponential scaling (i.e SlotValue = ezMath::Pow(SlotValue, -ScaleValue)). Default is 1.0f.
  float m_fInputSlotScale[MaxInputSlotAlternatives];

  /// \brief For Input Areas: If this is set, the input slot with the given name must have a value between m_fFilterXMinValue and m_fFilterXMaxValue. Otherwise this action will not be triggered.
  ezString m_sFilterByInputSlotX[MaxInputSlotAlternatives];

  /// \brief For Input Areas: If this is set, the input slot with the given name must have a value between m_fFilterYMinValue and m_fFilterYMaxValue. Otherwise this action will not be triggered.
  ezString m_sFilterByInputSlotY[MaxInputSlotAlternatives];

  float m_fFilterXMinValue; ///< =0; see m_sFilterByInputSlotX
  float m_fFilterXMaxValue; ///< =1; see m_sFilterByInputSlotX
  float m_fFilterYMinValue; ///< =0; see m_sFilterByInputSlotY
  float m_fFilterYMaxValue; ///< =1; see m_sFilterByInputSlotY

  float m_fFilteredPriority; ///< =large negative value; For Input Areas: If two input actions overlap and they have different priorities, the one with the larger priority will be triggered. Otherwise both are triggered.

  /// \brief For Input Areas: Describes what happens when an action is currently triggered, but the input slots used for filtering leave their min/max values.
  enum OnLeaveArea
  {
    LoseFocus,  ///< The input action will lose focus and thus get 'deactivated' immediately. Ie. it will return ezKeyState::Released.
    KeepFocus,  ///< The input action will keep focus and continue to return ezKeyState::Down, until all trigger slots are actually released.
  };

  /// \brief For Input Areas: Describes what happens when any trigger slot is already active will the input slots that are used for filtering enter the valid ranges.
  enum OnEnterArea
  {
    ActivateImmediately,  ///< The input action will immediately get activated and return ezKeyState::Pressed, even though the input slots are already pressed for some time.
    RequireKeyUp,         ///< The input action will not get triggered, unless some trigger input slot is actually pressed while the input slots that are used for filtering are actually within valid ranges.
  };

  OnLeaveArea m_OnLeaveArea; ///< =LoseFocus
  OnEnterArea m_OnEnterArea; ///< =ActivateImmediately
};


/// \brief The central class to set up and query the state of all input.
///
/// The ezInputManager is the central hub through which you can configure which keys will trigger which actions. You can query in which state
/// an action is (inactive (up), active (down), just recently activated (pressed) or just recently deactivated (released)). You can query their
/// values (e.g. how much a thumb-stick or the mouse was moved).
/// Additionally you can localize buttons and actions. The internal data will always use English names and the US keyboard layout, but what with which
/// names those keys are presented to the user can be changed.
/// Although the input manager allows to query the state of each key, button, axis, etc. directly, this is not advised. Instead the user should set
/// up 'actions' and define which keys will trigger those actions. At runtime the user should only query the state of actions. In the best case, an
/// application allows the player to change the mapping which keys are used to trigger which actions.
class EZ_CORE_DLL ezInputManager
{
public:

  /// \brief Updates the state of the input manager. This should be called exactly once each frame.
  ///
  /// \param tTimeDifference The time elapsed since the last update. This will affect the value scaling of actions that
  /// use frame time scaling and is necessary to update controller vibration tracks.
  static void Update(ezTime tTimeDifference); // [tested]

  /// \brief Changes the display name of an input slot.
  static void SetInputSlotDisplayName(const char* szInputSlot, const char* szDefaultDisplayName); // [tested]

  /// \brief Returns the display name that was assigned to the given input slot.
  static const char* GetInputSlotDisplayName(const char* szInputSlot); // [tested]

  /// \brief Sets the dead zone for the given input slot. As long as the hardware reports values lower than this, the input slot will report a value of zero.
  static void SetInputSlotDeadZone(const char* szInputSlot, float fDeadZone); // [tested]

  /// \brief Returns the dead zone value for the given input slot.
  static float GetInputSlotDeadZone(const char* szInputSlot); // [tested]

  /// \brief Returns the flags for the given input slot.
  static ezBitflags<ezInputSlotFlags> GetInputSlotFlags(const char* szInputSlot); // [tested]

  /// \brief Returns the current key state of the given input slot and optionally also returns its full value.
  ///
  /// Do not use this function, unless you really, really need the value of exactly this key.
  /// Prefer to map your key to an action and then use GetInputActionState(). That method is more robust and extensible.
  static ezKeyState::Enum GetInputSlotState(const char* szInputSlot, float* pValue = nullptr); // [tested]

  /// \brief Returns an array that contains all the names of all currently known input slots.
  static void RetrieveAllKnownInputSlots(ezDynamicArray<const char*>& out_InputSlots);

  /// \brief Returns the last typed character as the OS has reported it. Thus supports Unicode etc.
  ///
  /// If \a bResetCurrent is true, the internal last character will be reset to '\0'.
  /// If it is false, the internal state will not be changed. This should only be used, if the calling code does not do anything meaningful with the value.
  static ezUInt32 RetrieveLastCharacter(bool bResetCurrent = true); // [tested]

  /// \brief Makes sure that hardware input is processed at this moment, which allows to do this more often than Update() is called.
  ///
  /// When you have a game where you are doing relatively few game updates (including processing input), for example only 20 times
  /// per second, it is possible to 'miss' input. PollHardware() allows to introduce sampling the hardware state more often to prevent this.
  /// E.g. when your renderer renders at 60 Hz, you can poll input also at 60 Hz, even though you really only process it at 20 Hz.
  /// In typical usage scenarios this is not required to do and can be ignored.
  /// Note that you can call PollHardware() as often as you like and at irregular intervals, it will not have a negative effect
  /// on the input states.
  static void PollHardware();

  /// \brief If \a szInputSlot is used in any action in \a szInputSet, it will be removed from all of them.
  ///
  /// This should be used to reset the usage of an input slot before it is bound to another input action.
  static void ClearInputMapping(const char* szInputSet, const char* szInputSlot); // [tested]

  /// \brief This is the one function to set up which input actions are available and by which input slots (keys) they are triggered.
  ///
  /// \param szInputSet
  ///   'Input Sets' are sets of actions that are disjunct from each other. That means the same input slot (key, mouse button, etc.) can trigger
  ///   multiple different actions from different input sets. For example In the input set 'Game' the left mouse button may trigger the action
  ///   'Shoot', but in the input set 'UI' the left mouse button may trigger the action 'Click'. All input sets are always evaluated and update their
  ///   state simultaneously. The user only has to decide which actions to react to, ie. whether the game is currently running and thus the 'Game' input
  ///   set is queried or whether a menu is shown and thus the 'UI' input set is queried.
  /// \param szAction
  ///   The action that is supposed to be triggered. The same action name may be reused in multiple input sets, they will have nothing in common.
  ///   The action name should describe WHAT is to be done, not which key the user pressed. For example an action could be 'player_forwards'. Which
  ///   key is set to trigger that action should be irrelevant at run-time.
  /// \param Config
  ///   This struct defines exactly which input slots (keys, buttons etc.) will trigger this action. The configuration allows to scale key values by
  ///   the frame time, to get smooth movement when the frame-rate varies. It allows to only accept input from a slot if two other slots have certain
  ///   values. This makes it possible to react to mouse or touch input only if that input is done inside a certain input area.
  ///   The action can be triggered by multiple keys, if desired.
  ///   In the most common cases, one will only set one or two input slots as triggers (Config.m_sInputSlotTrigger) and possibly decide whether
  ///   frame time scaling is required. It makes sense to let the ezInputManager do the frame time scaling, because it should not be applied to all
  ///   input, e.g. mouse delta values should never be scaled by the frame time.
  /// \param bClearPreviousInputMappings
  ///   If set to true it is ensured that all the input slots that are used by this action are not mapped to any other action.
  ///   That means no other action can be triggered by this key within this input set.
  ///   For most actions this should be set to true. However, if you have several actions that can be triggered by the same slot
  ///   (for example touch input) but only in different areas of the screen, this should be set to false.
  static void SetInputActionConfig(const char* szInputSet, const char* szAction, const ezInputActionConfig& Config, bool bClearPreviousInputMappings); // [tested]

  /// \brief Returns the configuration for the given input action in the given input set. Returns a default configuration, if the action does not exist.
  static ezInputActionConfig GetInputActionConfig(const char* szInputSet, const char* szAction); // [tested]

  /// \brief Deletes all state associated with the given input action.
  ///
  /// It is not necessary to call this function for cleanup.
  static void RemoveInputAction(const char* szInputSet, const char* szAction); // [tested]

  /// \brief Returns the current state and value of the given input action.
  ///
  /// This is the one function that is called repeatedly at runtime to figure out which actions are active and thus which game-play functions
  /// to execute. You can (and should) use the /a pValue to scale game play features (e.g. how fast to drive).
  static ezKeyState::Enum GetInputActionState(const char* szInputSet, const char* szAction, float* pValue = nullptr, ezInt8* iTriggeredSlot = nullptr); // [tested]

  /// \brief Sets the display name for the given action.
  static void SetActionDisplayName(const char* szAction, const char* szDisplayName); // [tested]

  /// \brief Returns the display name for the given action, or the action name itself, if no special display name was specified yet.
  static const char* GetActionDisplayName(const char* szAction); // [tested]

  /// \brief Returns the names of all currently registered input sets.
  static void GetAllInputSets(ezDynamicArray<ezString>& out_InputSetNames); // [tested]

  /// \brief Returns the names of all input actions in the given input set.
  static void GetAllInputActions(const char* szInputSetName, ezDynamicArray<ezString>& out_InputActions); // [tested]

  /// \brief This function allows to 'inject' input state for one frame.
  ///
  /// This can be useful to emulate certain keys, e.g. for virtual devices.
  /// Note that it usually makes more sense to actually have another input device, however this can be used to
  /// get data into the system quickly for when a full blown input device might be overkill.
  /// The injected input state is cleared immediately after it has been processed, so to keep a virtual input slot active,
  /// the input needs to be injected every frame.
  ///
  /// Note that when the input is injected after ezInputManager::Update was called, its effect will be delayed by one frame.
  static void InjectInputSlotValue(const char* szInputSlot, float fValue); // [tested]

  /// \brief Checks whether any input slot has been triggered in this frame, which has all \a MustHaveFlags and has none of the \a MustNotHaveFlags.
  ///
  /// This function can be used in a UI to wait for user input and then assign that input to a certain action.
  static const char* GetPressedInputSlot(ezInputSlotFlags::Enum MustHaveFlags, ezInputSlotFlags::Enum MustNotHaveFlags); // [tested]

  /// \brief Mostly for internal use. Converts a scan-code value to the string that is used inside the engine for that key.
  static const char* ConvertScanCodeToEngineName(ezUInt8 uiScanCode, bool bIsExtendedKey);

  /// \brief The data that is broadcast when certain events occur.
  struct InputEventData
  {
    enum EventType
    {
      InputSlotChanged,   ///< An input slot has been registered or its state changed.
      InputActionChanged, ///< An input action has been registered or its state changed.
    };

    EventType m_EventType;
    const char* m_szInputSlot;
    const char* m_szInputSet;
    const char* m_szInputAction;

    InputEventData()
    {
      m_szInputSlot = nullptr;
      m_szInputSet = nullptr;
      m_szInputAction = nullptr;
    }
  };

  typedef ezEvent<const InputEventData&> ezEventInput;

  /// \brief Adds an event handler that is called for input events.
  static void AddEventHandler(ezEventInput::Handler handler)    { s_InputEvents.AddEventHandler   (handler);  }

  /// \brief Removes a previously added event handler.
  static void RemoveEventHandler(ezEventInput::Handler handler) { s_InputEvents.RemoveEventHandler (handler); }

private:
  friend class ezInputDevice;

  /// \brief Registers an input slot with the given name and a default display name.
  ///
  /// If the input slot was already registered before, the display name is only changed, it the previous registration did not
  /// specify a display name different from the input slot name.
  static void RegisterInputSlot(const char* szName, const char* szDefaultDisplayName, ezBitflags<ezInputSlotFlags> SlotFlags);


private:

  /// \brief Stores the current state for one input slot.
  struct ezInputSlot
  {
    ezInputSlot();

    ezString m_sDisplayName;                  ///< The display name. Use this to present input slots in UIs.
    float m_fValue;                           ///< The current value.
    ezKeyState::Enum m_State;                 ///< The current state.
    float m_fDeadZone;                        ///< The dead zone. Unless the value exceeds this, it reports a zero value.
    ezBitflags<ezInputSlotFlags> m_SlotFlags; ///< Describes the capabilities of the slot.
  };

  /// \brief The data that is stored for each action.
  struct ezActionData
  {
    ezActionData();

    ezInputActionConfig m_Config; ///< The configuration that was specified by the user.

    float m_fValue;               ///< The current value. This is the maximum of all input slot values that trigger this action.
    ezKeyState::Enum m_State;     ///< The current state. Derived from m_fValue.

    ezInt8 m_iTriggeredViaAlternative;
  };

  typedef ezMap<ezString, ezActionData> ezActionMap;    ///< Maps input action names to their data.
  typedef ezMap<ezString, ezActionMap> ezInputSetMap;   ///< Maps input set names to their data.
  typedef ezMap<ezString, ezInputSlot> ezInputSlotsMap; ///< Maps input slot names to their data.

  /// \brief The internal data of the ezInputManager. Not allocated until it is actually required.
  struct InternalData
  {
    ezInputSetMap s_ActionMapping;                      ///< Maps input set names to their data.
    ezInputSlotsMap s_InputSlots;                       ///< Maps input slot names to their data.
    ezMap<ezString, ezString> s_ActionDisplayNames;     ///< Stores a display name for each input action.
    ezMap<ezString, float> s_InjectedInputSlots;
  };

  /// \brief The last (Unicode) character that was typed by the user, as reported by the OS (on Windows: WM_CHAR).
  static ezUInt32 s_LastCharacter;

  static bool s_bInputSlotResetRequired;

  /// \brief Resets all input slot value to zero.
  static void ResetInputSlotValues();

  /// \brief Queries all known devices for their input slot values and stores the maximum values inside the input manager.
  static void GatherDeviceInputSlotValues();

  /// \brief Uses the previously queried input slot values to update their overall state.
  static void UpdateInputSlotStates();

  /// \brief Uses the previously queried input slot values to update the state (and value) of all input actions.
  static void UpdateInputActions(ezTime tTimeDifference);

  /// \brief Updates the state of all input actions in the given input set.
  static void UpdateInputActions(const char* szInputSet, ezActionMap& Actions, ezTime tTimeDifference);

  /// \brief Returns an iterator to the (next) action that should get triggered by the given slot.
  ///
  /// This may return several actions (when called repeatedly) when their are several actions with the same priority.
  static ezActionMap::Iterator GetBestAction(ezActionMap& Actions, const ezString& sSlot, const ezActionMap::Iterator& itFirst);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, InputManager);

  static void DeallocateInternals();
  static InternalData& GetInternals();

  static ezEventInput s_InputEvents;

  static InternalData* s_pData;
};



