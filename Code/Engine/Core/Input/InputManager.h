#pragma once

#include <Core/Input/InputDevice.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/DynamicArray.h>

EZ_CORE_DLL const char* ConvertScanCodeToEngineName(ezUInt8 uiScanCode, bool bIsExtendedKey);

class EZ_CORE_DLL ezInputManager
{
public:

  static ezInputDevice* GetInputDeviceByType(const char* szType);
  static ezInputDevice* GetInputDeviceByName(const char* szName);

  static void SetInputSlotDisplayName(const char* szInputSlot, const char* szDefaultDisplayName);

  static const char* GetInputSlotDisplayName(const char* szInputSlot);

  static void SetInputSlotDeadZone(const char* szInputSlot, float fDeadZone);
  static float GetInputSlotDeadZone(const char* szInputSlot);

  static void SetInputSlotScale(const char* szInputSlot, float fScale);
  static float GetInputSlotScale(const char* szInputSlot);

  static ezKeyState::Enum GetInputSlotState(const char* szInputSlot, float* pValue = NULL);

  static void RetrieveAllKnownInputSlots(ezDynamicArray<const char*>& out_InputSlots);

  static void Update();

  /// \brief Returns the last typed character as the OS has reported it. Thus supports unicode etc.
  ///
  /// If \a bResetCurrent is true, the internal last character will be reset to '\0'.
  /// If it is false, the internal state will not be changed. This should only be used, if the calling code does not do anything meaningful with the value.
  static wchar_t RetrieveLastCharacter(bool bResetCurrent = true);

  struct ezInputActionConfig
  {
    enum { MaxInputSlotAlternatives = 3 };

    ezInputActionConfig();

    ezString m_sInputSlotTrigger[MaxInputSlotAlternatives];

    ezString m_sFilterByInputSlotX;
    ezString m_sFilterByInputSlotY;

    float m_fFilterXMinValue; ///< =0
    float m_fFilterXMaxValue; ///< =1
    float m_fFilterYMinValue; ///< =0
    float m_fFilterYMaxValue; ///< =1

    float m_fFilteredPriority; ///< =large negative value

    enum OnLeaveArea
    {
      LoseFocus,
      KeepFocus,
    };

    enum OnEnterArea
    {
      ActivateImmediately,
      RequireKeyUp,
    };

    OnLeaveArea m_OnLeaveArea; ///< =LoseFocus
    OnEnterArea m_OnEnterArea; ///< =ActivateImmediately
  };

  static void ClearInputMapping(const char* szInputSet, const char* szInputSlot);
  static void SetInputActionConfig(const char* szInputSet, const char* szAction, const ezInputActionConfig& Config, bool bClearPreviousInputMappings);
  static ezInputActionConfig GetInputActionConfig(const char* szInputSet, const char* szAction);
  static void RemoveInputAction(const char* szInputSet, const char* szAction);
  static ezKeyState::Enum GetInputActionState(const char* szInputSet, const char* szAction, float* pValue = NULL);

  static void SetActionDisplayName(const char* szAction, const char* szDisplayName);
  static const char* GetActionDisplayName(const char* szAction);

  static void GetAllInputSets(ezDynamicArray<ezString>& out_InputSetNames);
  static void GetAllInputActions(const char* szInputSetName, ezDynamicArray<ezString>& out_InputActions);

private:
  friend class ezInputDevice;

  static void RegisterInputSlot(const char* szName, const char* szDefaultDisplayName);


private:
  static void ResetInputSlotValues();
  static void GatherDeviceInputSlotValues();
  static void UpdateInputSlotStates();
  static void UpdateInputActions();

  struct ezInputSlot
  {
    ezInputSlot();

    ezString m_sDisplayName;
    float m_fValue;
    ezKeyState::Enum m_State;
    float m_fDeadZone;
    float m_fScale;
  };

  struct ezActionData
  {
    ezActionData();

    ezInputActionConfig m_Config;

    float m_fValue;
    ezKeyState::Enum m_State;
  };

  typedef ezMap<ezString, ezActionData> ezActionMap;
  typedef ezMap<ezString, ezActionMap, ezCompareHelper<ezString>, ezStaticAllocatorWrapper > ezInputSetMap; 

  static void UpdateInputActions(ezActionMap& Actions);

  static ezInputSetMap s_ActionMapping;

  static wchar_t s_LastCharacter;

  typedef ezMap<ezString, ezInputSlot, ezCompareHelper<ezString>, ezStaticAllocatorWrapper> ezInputSlotsMap;

  static ezInputSlotsMap s_InputSlots;

  static ezActionMap::Iterator GetBestAction(ezActionMap& Actions, const ezString& sSlot, const ezActionMap::Iterator& itFirst);

  static ezMap<ezString, ezString, ezCompareHelper<ezString>, ezStaticAllocatorWrapper> s_ActionDisplayNames; 
};



