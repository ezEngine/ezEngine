#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>
#include <Core/Input/Declarations.h>

struct ezRmlUiInputButtons
{
  using StorageType = ezUInt32;

  enum Enum
  {
    None             = 0,

    Mouse0           = EZ_BIT(0),
    Mouse1           = EZ_BIT(1),
    Mouse2           = EZ_BIT(2),
    MouseWheelUp     = EZ_BIT(3),
    MouseWheelDown   = EZ_BIT(4),
    Tab              = EZ_BIT(5),
    Left             = EZ_BIT(6),
    Up               = EZ_BIT(7),
    Right            = EZ_BIT(8),
    Down             = EZ_BIT(9),
    PageUp           = EZ_BIT(10),
    PageDown         = EZ_BIT(11),
    Home             = EZ_BIT(12),
    End              = EZ_BIT(13),
    Delete           = EZ_BIT(14),
    Backspace        = EZ_BIT(15),
    Return           = EZ_BIT(16),
    NumpadEnter      = EZ_BIT(17),
    Escape           = EZ_BIT(18),
    Alt              = EZ_BIT(19),
    Ctrl             = EZ_BIT(20),
    Shift            = EZ_BIT(21),

    Default          = None,
  };

  struct Bits
  {
    StorageType Mouse0          : 1;
    StorageType Mouse1          : 1;
    StorageType Mouse2          : 1;
    StorageType MouseWheelUp    : 1;
    StorageType MouseWheelDown  : 1;
    StorageType Tab             : 1;
    StorageType Left            : 1;
    StorageType Up              : 1;
    StorageType Right           : 1;
    StorageType Down            : 1;
    StorageType PageUp          : 1;
    StorageType PageDown        : 1;
    StorageType Home            : 1;
    StorageType End             : 1;
    StorageType Delete          : 1;
    StorageType Backspace       : 1;
    StorageType Return          : 1;
    StorageType NumpadEnter     : 1;
    StorageType Escape          : 1;
    StorageType Alt             : 1;
    StorageType Ctrl            : 1;
    StorageType Shift           : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezRmlUiInputButtons);

struct EZ_RMLUIPLUGIN_DLL ezRmlUiInputSnapshot
{
  ezBitflags<ezRmlUiInputButtons> m_Buttons;
  ezUInt32 m_uiLastCharacter = 0;

  EZ_ALWAYS_INLINE bool operator==(const ezRmlUiInputSnapshot& rhs) const
  {
    return m_Buttons == rhs.m_Buttons && m_uiLastCharacter == rhs.m_uiLastCharacter;
  }

  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezRmlUiInputSnapshot&);
};

struct EZ_RMLUIPLUGIN_DLL ezRmlUiInputProvider
{
  EZ_ALWAYS_INLINE bool Update(ezRmlUiInputSnapshot input)
  {
    bool bHasChanged = m_Buttons != input.m_Buttons || m_uiLastCharacter != input.m_uiLastCharacter;
    m_PrevButtons = m_Buttons;
    m_Buttons = input.m_Buttons;
    m_uiLastCharacter = input.m_uiLastCharacter;
    return bHasChanged;
  }

  EZ_ALWAYS_INLINE ezKeyState::Enum GetButtonState(ezRmlUiInputButtons::Enum button) const
  {
    if (m_Buttons.IsSet(button))
    {
      if (!m_PrevButtons.IsSet(button))
        return ezKeyState::Pressed;
      return ezKeyState::Down;
    }
    else
    {
      if (m_PrevButtons.IsSet(button))
        return ezKeyState::Released;
      return ezKeyState::Up;
    }
  }

  EZ_ALWAYS_INLINE bool IsButtonDown(ezRmlUiInputButtons::Enum button) const
  {
    return m_Buttons.IsSet(button);
  }

  EZ_ALWAYS_INLINE bool IsAnyButtonDown() const
  {
    return m_Buttons.IsAnyFlagSet() || m_uiLastCharacter != 0;
  }

  EZ_ALWAYS_INLINE bool HasAnyInput() const
  {
    return IsAnyButtonDown() || m_PrevButtons.IsAnyFlagSet();
  }

  ezUInt32 m_uiLastCharacter = 0;
  ezBitflags<ezRmlUiInputButtons> m_Buttons, m_PrevButtons;
};
