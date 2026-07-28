#include <RTSPlugin/RTSPluginPCH.h>

#include <RTSPlugin/GameState/RTSGameState.h>

// Demonstrates ezInputManager::SetMouseCursor() and the ezMouseCursorRenderer that draws it.
//
// The game only describes what the cursor should look like. It does not create any game object and
// it does not care where or when the cursor is rendered - that is entirely up to the engine, which
// draws it on top of everything else, even while no world is loaded.

// Textures/Cursor_Arrow.ezTextureAsset
static constexpr ezStringView s_sCursorArrow = "{ d4aca571-6900-48aa-93c6-3d388f551f50 }"_ezsv;

void RTSGameState::UpdateMouseCursor()
{
  ezMouseCursorDesc cursor;
  cursor.m_sCursor = s_sCursorArrow;

  // m_fSize is left at one, so the cursor automatically matches the size of the OS cursor,
  // no matter the resolution, window size or DPI scaling.

  // The arrow's tip is in the top-left corner of the image.
  cursor.m_vHotspot.Set(0.0f);

  switch (GetActiveGameMode())
  {
    case RtsActiveGameMode::BattleMode:
      // Spin the cursor while the right mouse button is held down, to show that animating the
      // cursor every frame is free - only the values below are updated, nothing is re-created.
      if (m_MouseInputState.m_RightClickState >= ezKeyState::Pressed)
      {
        m_CursorAnimation += ezClock::GetGlobalClock()->GetTimeDiff();
        cursor.m_Rotation = ezAngle::MakeFromDegree((float)m_CursorAnimation.GetSeconds() * 180.0f);
      }
      else
      {
        m_CursorAnimation = ezTime::MakeZero();
      }

      // Tint the cursor while something is selected, to show off the color modulation.
      if (m_SelectedUnits.GetCount() > 0)
      {
        cursor.m_Color = ezColor::OrangeRed;
      }
      break;

    default:
      break;
  }

  ezInputManager::SetMouseCursor(cursor);
}
