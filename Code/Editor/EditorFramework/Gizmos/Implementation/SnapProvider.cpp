#include <EditorFrameworkPCH.h>

#include <EditorFramework/Gizmos/SnapProvider.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>

ezAngle ezSnapProvider::s_RotationSnapValue = ezAngle::Degree(15.0f);
float ezSnapProvider::s_fScaleSnapValue = 0.125f;
float ezSnapProvider::s_fTranslationSnapValue = 0.25f;
ezEvent<const ezSnapProviderEvent&> ezSnapProvider::s_Events;


ezAngle ezSnapProvider::GetRotationSnapValue()
{
  return s_RotationSnapValue;
}

float ezSnapProvider::GetScaleSnapValue()
{
  return s_fScaleSnapValue;
}

float ezSnapProvider::GetTranslationSnapValue()
{
  return s_fTranslationSnapValue;
}

void ezSnapProvider::SetRotationSnapValue(ezAngle angle)
{
  if (s_RotationSnapValue == angle)
    return;

  s_RotationSnapValue = angle;

  ezSnapProviderEvent e;
  e.m_Type = ezSnapProviderEvent::Type::RotationSnapChanged;
  s_Events.Broadcast(e);
}

void ezSnapProvider::SetScaleSnapValue(float fPercentage)
{
  if (s_fScaleSnapValue == fPercentage)
    return;

  s_fScaleSnapValue = fPercentage;

  ezSnapProviderEvent e;
  e.m_Type = ezSnapProviderEvent::Type::ScaleSnapChanged;
  s_Events.Broadcast(e);
}

void ezSnapProvider::SetTranslationSnapValue(float fUnits)
{
  if (s_fTranslationSnapValue == fUnits)
    return;

  s_fTranslationSnapValue = fUnits;

  ezSnapProviderEvent e;
  e.m_Type = ezSnapProviderEvent::Type::TranslationSnapChanged;
  s_Events.Broadcast(e);
}

void ezSnapProvider::SnapTranslation(ezVec3& value)
{
  if (s_fTranslationSnapValue <= 0.0f)
    return;

  value.x = ezMath::RoundToMultiple(value.x, s_fTranslationSnapValue);
  value.y = ezMath::RoundToMultiple(value.y, s_fTranslationSnapValue);
  value.z = ezMath::RoundToMultiple(value.z, s_fTranslationSnapValue);
}

void ezSnapProvider::SnapTranslationInLocalSpace(const ezQuat& rotation, ezVec3& translation)
{
  if (s_fTranslationSnapValue <= 0.0f)
    return;

  const ezQuat mInvRot = -rotation;

  ezVec3 vLocalTranslation = mInvRot * translation;
  vLocalTranslation.x = ezMath::RoundToMultiple(vLocalTranslation.x, s_fTranslationSnapValue);
  vLocalTranslation.y = ezMath::RoundToMultiple(vLocalTranslation.y, s_fTranslationSnapValue);
  vLocalTranslation.z = ezMath::RoundToMultiple(vLocalTranslation.z, s_fTranslationSnapValue);

  translation = rotation * vLocalTranslation;
}

void ezSnapProvider::SnapRotation(ezAngle& rotation)
{
  if (s_RotationSnapValue.GetRadian() != 0.0f)
  {
    rotation = ezAngle::Radian(ezMath::RoundToMultiple(rotation.GetRadian(), s_RotationSnapValue.GetRadian()));
  }
}

void ezSnapProvider::SnapScale(float& scale)
{
  if (s_fScaleSnapValue > 0.0f)
  {
    scale = ezMath::RoundToMultiple(scale, s_fScaleSnapValue);
  }
}

void ezSnapProvider::SnapScale(ezVec3& scale)
{
  if (s_fScaleSnapValue > 0.0f)
  {
    SnapScale(scale.x);
    SnapScale(scale.y);
    SnapScale(scale.z);
  }
}

ezVec3 ezSnapProvider::GetScaleSnapped(const ezVec3& scale)
{
  ezVec3 res = scale;
  SnapScale(res);
  return res;
}
