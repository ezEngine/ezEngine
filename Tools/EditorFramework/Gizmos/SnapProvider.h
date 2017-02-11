#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Communication/Event.h>

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

  static ezAngle GetRotationSnapValue();
  static float GetScaleSnapValue();
  static float GetTranslationSnapValue();

  static void SetRotationSnapValue(ezAngle angle);
  static void SetScaleSnapValue(float fPercentage);
  static void SetTranslationSnapValue(float fUnits);

  /// \brief Rounds each component to the closest translation snapping value
  static void SnapTranslation(ezVec3& value);

  /// \brief Inverts the rotation, applies that to the translation, snaps it and then transforms it back into the original space
  static void SnapTranslationInLocalSpace(const ezMat3& rotation, ezVec3& translation);

  static void SnapRotation(ezAngle& rotation);

  static void SnapScale(float& scale);
  static void SnapScale(ezVec3& scale);

  static ezVec3 GetScaleSnapped(const ezVec3& scale);

  static ezEvent<const ezSnapProviderEvent&> s_Events;

private:
  static ezAngle s_RotationSnapValue;
  static float s_fScaleSnapValue;
  static float s_fTranslationSnapValue;
};
