#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Math/Declarations.h>

struct ezEditorAppEvent;
class ezPreferences;

struct ezSnapProviderEvent
{
  enum class Type
  {
    RotationSnapChanged,
    ScaleSnapChanged,
    TranslationSnapChanged
  };

  Type m_Type;
};

class EZ_EDITORFRAMEWORK_DLL ezSnapProvider
{
public:
  static void Startup();
  static void Shutdown();

  static ezAngle GetRotationSnapValue();
  static float GetScaleSnapValue();
  static float GetTranslationSnapValue();

  static void SetRotationSnapValue(ezAngle angle);
  static void SetScaleSnapValue(float fPercentage);
  static void SetTranslationSnapValue(float fUnits);

  /// \brief Rounds each component to the closest translation snapping value
  static void SnapTranslation(ezVec3& value);

  /// \brief Inverts the rotation, applies that to the translation, snaps it and then transforms it back into the original space
  static void SnapTranslationInLocalSpace(const ezQuat& qRotation, ezVec3& ref_vTranslation);

  static void SnapRotation(ezAngle& ref_rotation);

  static void SnapScale(float& ref_fScale);
  static void SnapScale(ezVec3& ref_vScale);

  static ezVec3 GetScaleSnapped(const ezVec3& vScale);

  static ezEvent<const ezSnapProviderEvent&> s_Events;

private:
  static void EditorEventHandler(const ezEditorAppEvent& e);
  static void PreferenceChangedEventHandler(ezPreferences* pPreferenceBase);

  static ezAngle s_RotationSnapValue;
  static float s_fScaleSnapValue;
  static float s_fTranslationSnapValue;
  static ezEventSubscriptionID s_UserPreferencesChanged;
};
