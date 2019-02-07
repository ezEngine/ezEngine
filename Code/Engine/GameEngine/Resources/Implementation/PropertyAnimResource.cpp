#include <GameEnginePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <GameEngine/Resources/PropertyAnimResource.h>

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
  res.m_uiQualityLevelsLoadable = 1;
  res.m_State = ezResourceState::Unloaded;

  m_pDescriptor = nullptr;

  return res;
}

ezResourceLoadDesc ezPropertyAnimResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezPropertyAnimResource::UpdateContent", GetResourceDescription().GetData());

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
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  {
    ezResourceHandleReadContext context;
    context.BeginReadingFromStream(Stream);
    context.BeginRestoringHandles(Stream);

    m_pDescriptor = EZ_DEFAULT_NEW(ezPropertyAnimResourceDescriptor);
    m_pDescriptor->Load(*Stream);

    context.EndReadingFromStream(Stream);
    context.EndRestoringHandles();
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezPropertyAnimResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = 0;

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU =
        static_cast<ezUInt32>(m_pDescriptor->m_FloatAnimations.GetHeapMemoryUsage() + sizeof(ezPropertyAnimResourceDescriptor));
  }
}

void ezPropertyAnimResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 6;
  const ezUInt8 uiIdentifier = 0x0A; // dummy to fill the header to 32 Bit
  const ezUInt16 uiNumFloatAnimations = m_FloatAnimations.GetCount();
  const ezUInt16 uiNumColorAnimations = m_ColorAnimations.GetCount();

  EZ_ASSERT_DEV(m_AnimationDuration.GetSeconds() > 0, "Animation duration must be positive");

  stream << uiVersion;
  stream << uiIdentifier;
  stream << m_AnimationDuration;
  stream << uiNumFloatAnimations;

  ezCurve1D tmpCurve;

  for (ezUInt32 i = 0; i < uiNumFloatAnimations; ++i)
  {
    stream << m_FloatAnimations[i].m_sObjectSearchSequence;
    stream << m_FloatAnimations[i].m_sComponentType;
    stream << m_FloatAnimations[i].m_sPropertyPath;
    stream << m_FloatAnimations[i].m_Target;

    tmpCurve = m_FloatAnimations[i].m_Curve;
    tmpCurve.SortControlPoints();
    tmpCurve.ApplyTangentModes();
    tmpCurve.ClampTangents();
    tmpCurve.Save(stream);
  }

  ezColorGradient tmpGradient;
  stream << uiNumColorAnimations;
  for (ezUInt32 i = 0; i < uiNumColorAnimations; ++i)
  {
    stream << m_ColorAnimations[i].m_sObjectSearchSequence;
    stream << m_ColorAnimations[i].m_sComponentType;
    stream << m_ColorAnimations[i].m_sPropertyPath;
    stream << m_ColorAnimations[i].m_Target;

    tmpGradient = m_ColorAnimations[i].m_Gradient;
    tmpGradient.SortControlPoints();
    tmpGradient.Save(stream);
  }

  // Version 6
  m_EventTrack.Save(stream);
}

void ezPropertyAnimResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  ezUInt8 uiIdentifier = 0;
  ezUInt16 uiNumAnimations = 0;

  stream >> uiVersion;
  stream >> uiIdentifier;

  EZ_ASSERT_DEV(uiIdentifier == 0x0A, "File does not contain a valid ezPropertyAnimResourceDescriptor");
  EZ_ASSERT_DEV(uiVersion == 4 || uiVersion == 5 || uiVersion == 6, "Invalid file version {0}", uiVersion);

  stream >> m_AnimationDuration;

  if (uiVersion == 4)
  {
    ezEnum<ezPropertyAnimMode> mode;
    stream >> mode;
  }

  stream >> uiNumAnimations;
  m_FloatAnimations.SetCount(uiNumAnimations);

  for (ezUInt32 i = 0; i < uiNumAnimations; ++i)
  {
    auto& anim = m_FloatAnimations[i];

    stream >> anim.m_sObjectSearchSequence;
    stream >> anim.m_sComponentType;
    stream >> anim.m_sPropertyPath;
    stream >> anim.m_Target;
    anim.m_Curve.Load(stream);
    anim.m_Curve.SortControlPoints();
    anim.m_Curve.CreateLinearApproximation();

    if (!anim.m_sComponentType.IsEmpty())
      anim.m_pComponentRtti = ezRTTI::FindTypeByName(anim.m_sComponentType);
  }

  stream >> uiNumAnimations;
  m_ColorAnimations.SetCount(uiNumAnimations);

  for (ezUInt32 i = 0; i < uiNumAnimations; ++i)
  {
    auto& anim = m_ColorAnimations[i];

    stream >> anim.m_sObjectSearchSequence;
    stream >> anim.m_sComponentType;
    stream >> anim.m_sPropertyPath;
    stream >> anim.m_Target;
    anim.m_Gradient.Load(stream);

    if (!anim.m_sComponentType.IsEmpty())
      anim.m_pComponentRtti = ezRTTI::FindTypeByName(anim.m_sComponentType);
  }

  if (uiVersion >= 6)
  {
    m_EventTrack.Load(stream);
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Resources_Implementation_PropertyAnimResource);

