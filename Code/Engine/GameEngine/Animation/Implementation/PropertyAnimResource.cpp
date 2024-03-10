#include <GameEngine/GameEnginePCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAnimResource, 1, ezRTTIDefaultAllocator<ezPropertyAnimResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPropertyAnimTarget, 1)
EZ_ENUM_CONSTANTS(ezPropertyAnimTarget::Number, ezPropertyAnimTarget::VectorX, ezPropertyAnimTarget::VectorY, ezPropertyAnimTarget::VectorZ, ezPropertyAnimTarget::VectorW)
EZ_ENUM_CONSTANTS(ezPropertyAnimTarget::RotationX, ezPropertyAnimTarget::RotationY, ezPropertyAnimTarget::RotationZ, ezPropertyAnimTarget::Color)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPropertyAnimMode, 1)
EZ_ENUM_CONSTANTS(ezPropertyAnimMode::Once, ezPropertyAnimMode::Loop, ezPropertyAnimMode::BackAndForth)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezPropertyAnimResource);
// clang-format on

ezPropertyAnimResource::ezPropertyAnimResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezPropertyAnimResource, ezPropertyAnimResourceDescriptor)
{
  m_pDescriptor = EZ_DEFAULT_NEW(ezPropertyAnimResourceDescriptor);
  *m_pDescriptor = descriptor;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

ezResourceLoadDesc ezPropertyAnimResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  m_pDescriptor = nullptr;

  return res;
}

ezResourceLoadDesc ezPropertyAnimResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezPropertyAnimResource::UpdateContent", GetResourceIdOrDescription());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = EZ_DEFAULT_NEW(ezPropertyAnimResourceDescriptor);
  m_pDescriptor->Load(*Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezPropertyAnimResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = 0;

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU = m_pDescriptor->m_FloatAnimations.GetHeapMemoryUsage() + sizeof(ezPropertyAnimResourceDescriptor);
  }
}

void ezPropertyAnimResourceDescriptor::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 6;
  const ezUInt8 uiIdentifier = 0x0A; // dummy to fill the header to 32 Bit
  const ezUInt16 uiNumFloatAnimations = static_cast<ezUInt16>(m_FloatAnimations.GetCount());
  const ezUInt16 uiNumColorAnimations = static_cast<ezUInt16>(m_ColorAnimations.GetCount());

  EZ_ASSERT_DEV(m_AnimationDuration.GetSeconds() > 0, "Animation duration must be positive");

  inout_stream << uiVersion;
  inout_stream << uiIdentifier;
  inout_stream << m_AnimationDuration;
  inout_stream << uiNumFloatAnimations;

  ezCurve1D tmpCurve;

  for (ezUInt32 i = 0; i < uiNumFloatAnimations; ++i)
  {
    inout_stream << m_FloatAnimations[i].m_sObjectSearchSequence;
    inout_stream << m_FloatAnimations[i].m_sComponentType;
    inout_stream << m_FloatAnimations[i].m_sPropertyPath;
    inout_stream << m_FloatAnimations[i].m_Target;

    tmpCurve = m_FloatAnimations[i].m_Curve;
    tmpCurve.SortControlPoints();
    tmpCurve.ApplyTangentModes();
    tmpCurve.ClampTangents();
    tmpCurve.Save(inout_stream);
  }

  ezColorGradient tmpGradient;
  inout_stream << uiNumColorAnimations;
  for (ezUInt32 i = 0; i < uiNumColorAnimations; ++i)
  {
    inout_stream << m_ColorAnimations[i].m_sObjectSearchSequence;
    inout_stream << m_ColorAnimations[i].m_sComponentType;
    inout_stream << m_ColorAnimations[i].m_sPropertyPath;
    inout_stream << m_ColorAnimations[i].m_Target;

    tmpGradient = m_ColorAnimations[i].m_Gradient;
    tmpGradient.SortControlPoints();
    tmpGradient.Save(inout_stream);
  }

  // Version 6
  m_EventTrack.Save(inout_stream);
}

void ezPropertyAnimResourceDescriptor::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  ezUInt8 uiIdentifier = 0;
  ezUInt16 uiNumAnimations = 0;

  inout_stream >> uiVersion;
  inout_stream >> uiIdentifier;

  EZ_ASSERT_DEV(uiIdentifier == 0x0A, "File does not contain a valid ezPropertyAnimResourceDescriptor");
  EZ_ASSERT_DEV(uiVersion == 4 || uiVersion == 5 || uiVersion == 6, "Invalid file version {0}", uiVersion);

  inout_stream >> m_AnimationDuration;

  if (uiVersion == 4)
  {
    ezEnum<ezPropertyAnimMode> mode;
    inout_stream >> mode;
  }

  inout_stream >> uiNumAnimations;
  m_FloatAnimations.SetCount(uiNumAnimations);

  for (ezUInt32 i = 0; i < uiNumAnimations; ++i)
  {
    auto& anim = m_FloatAnimations[i];

    inout_stream >> anim.m_sObjectSearchSequence;
    inout_stream >> anim.m_sComponentType;
    inout_stream >> anim.m_sPropertyPath;
    inout_stream >> anim.m_Target;
    anim.m_Curve.Load(inout_stream);
    anim.m_Curve.SortControlPoints();
    anim.m_Curve.CreateLinearApproximation();

    if (!anim.m_sComponentType.IsEmpty())
      anim.m_pComponentRtti = ezRTTI::FindTypeByName(anim.m_sComponentType);
  }

  inout_stream >> uiNumAnimations;
  m_ColorAnimations.SetCount(uiNumAnimations);

  for (ezUInt32 i = 0; i < uiNumAnimations; ++i)
  {
    auto& anim = m_ColorAnimations[i];

    inout_stream >> anim.m_sObjectSearchSequence;
    inout_stream >> anim.m_sComponentType;
    inout_stream >> anim.m_sPropertyPath;
    inout_stream >> anim.m_Target;
    anim.m_Gradient.Load(inout_stream);

    if (!anim.m_sComponentType.IsEmpty())
      anim.m_pComponentRtti = ezRTTI::FindTypeByName(anim.m_sComponentType);
  }

  if (uiVersion >= 6)
  {
    m_EventTrack.Load(inout_stream);
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_PropertyAnimResource);
