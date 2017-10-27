#include <PCH.h>
#include <GameEngine/Resources/PropertyAnimResource.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <GameEngine/Curves/ColorGradientResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAnimResource, 1, ezRTTIDefaultAllocator<ezPropertyAnimResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPropertyAnimTarget, 1)
EZ_ENUM_CONSTANTS(ezPropertyAnimTarget::Number, ezPropertyAnimTarget::VectorX, ezPropertyAnimTarget::VectorY, ezPropertyAnimTarget::VectorZ, ezPropertyAnimTarget::VectorW, ezPropertyAnimTarget::Color)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPropertyAnimMode, 1)
EZ_ENUM_CONSTANTS(ezPropertyAnimMode::Once, ezPropertyAnimMode::Loop, ezPropertyAnimMode::BackAndForth)
EZ_END_STATIC_REFLECTED_ENUM()

ezPropertyAnimResource::ezPropertyAnimResource()
  : ezResource<ezPropertyAnimResource, ezPropertyAnimResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezPropertyAnimResource::CreateResource(const ezPropertyAnimResourceDescriptor& descriptor)
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
    out_NewMemoryUsage.m_uiMemoryCPU = static_cast<ezUInt32>(m_pDescriptor->m_FloatAnimations.GetHeapMemoryUsage() + sizeof(ezPropertyAnimResourceDescriptor));
  }
}

void ezPropertyAnimResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 2;
  const ezUInt8 uiIdentifier = 0x0A; // dummy to fill the header to 32 Bit
  const ezUInt16 uiNumFloatAnimations = m_FloatAnimations.GetCount();
  const ezUInt16 uiNumColorAnimations = m_ColorAnimations.GetCount();

  stream << uiVersion;
  stream << uiIdentifier;
  stream << m_AnimationDuration;
  stream << m_Mode;
  stream << uiNumFloatAnimations;

  for (ezUInt32 i = 0; i < uiNumFloatAnimations; ++i)
  {
    stream << m_FloatAnimations[i].m_sPropertyName;
    stream << m_FloatAnimations[i].m_Target;
    m_FloatAnimations[i].m_Curve.Save(stream);
  }

  stream << uiNumColorAnimations;
  for (ezUInt32 i = 0; i < uiNumColorAnimations; ++i)
  {
    stream << m_ColorAnimations[i].m_sPropertyName;
    stream << m_ColorAnimations[i].m_Target;
    m_ColorAnimations[i].m_Gradient.Save(stream);
  }
}

void ezPropertyAnimResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  ezUInt8 uiIdentifier = 0;
  ezUInt16 uiNumFloatAnimations = 0;
  ezUInt16 uiNumColorAnimations = 0;

  stream >> uiVersion;
  stream >> uiIdentifier;
  stream >> m_AnimationDuration;
  stream >> m_Mode;

  EZ_ASSERT_DEV(uiIdentifier == 0x0A, "File does not contain a valid ezPropertyAnimResourceDescriptor");
  EZ_ASSERT_DEV(uiVersion == 2, "Invalid file version {0}", uiVersion);

  stream >> uiNumFloatAnimations;
  m_FloatAnimations.SetCount(uiNumFloatAnimations);

  for (ezUInt32 i = 0; i < uiNumFloatAnimations; ++i)
  {
    stream >> m_FloatAnimations[i].m_sPropertyName;
    stream >> m_FloatAnimations[i].m_Target;
    m_FloatAnimations[i].m_Curve.Load(stream);
  }

  stream >> uiNumColorAnimations;
  m_ColorAnimations.SetCount(uiNumColorAnimations);

  for (ezUInt32 i = 0; i < uiNumColorAnimations; ++i)
  {
    stream >> m_ColorAnimations[i].m_sPropertyName;
    stream >> m_ColorAnimations[i].m_Target;
    m_ColorAnimations[i].m_Gradient.Load(stream);
  }
}

