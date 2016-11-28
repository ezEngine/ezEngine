#include <Animation/PCH.h>
#include <Animation/Resources/PropertyAnimResource.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <GameUtils/Curves/Curve1DResource.h>
#include <GameUtils/Curves/ColorGradientResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAnimResource, 1, ezRTTIDefaultAllocator<ezPropertyAnimResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPropertyAnimTarget, 1)
EZ_ENUM_CONSTANTS(ezPropertyAnimTarget::Number, ezPropertyAnimTarget::VectorX, ezPropertyAnimTarget::VectorY, ezPropertyAnimTarget::VectorZ, ezPropertyAnimTarget::VectorW, ezPropertyAnimTarget::Color)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPropertyAnimMode, 1)
EZ_ENUM_CONSTANTS(ezPropertyAnimMode::Once, ezPropertyAnimMode::Loop, ezPropertyAnimMode::BackAndForth)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezPropertyAnimEntry, ezNoBase, 1, ezRTTIDefaultAllocator<ezPropertyAnimEntry>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Property", m_sPropertyName),
    EZ_MEMBER_PROPERTY("Duration", m_Duration)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(5))),
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezPropertyAnimMode, m_Mode),
    EZ_ENUM_MEMBER_PROPERTY("Target", ezPropertyAnimTarget, m_Target),
    EZ_ACCESSOR_PROPERTY("NumberCurve", GetNumberCurveFile, SetNumberCurveFile)->AddAttributes(new ezAssetBrowserAttribute("Curve1D")),
    EZ_ACCESSOR_PROPERTY("ColorGradient", GetColorCurveFile, SetColorCurveFile)->AddAttributes(new ezAssetBrowserAttribute("ColorGradient")),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE


EZ_BEGIN_STATIC_REFLECTED_TYPE(ezPropertyAnimResourceDescriptor, ezNoBase, 1, ezRTTIDefaultAllocator<ezPropertyAnimResourceDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Animations", m_Animations),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE


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
    out_NewMemoryUsage.m_uiMemoryCPU = static_cast<ezUInt32>(m_pDescriptor->m_Animations.GetHeapMemoryUsage() + sizeof(ezPropertyAnimResourceDescriptor));
  }
}

void ezPropertyAnimResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  const ezUInt8 uiIdentifier = 0x0A; // dummy to fill the header to 32 Bit
  const ezUInt16 uiNumAnimations = m_Animations.GetCount();

  stream << uiVersion;
  stream << uiIdentifier;
  stream << uiNumAnimations;

  for (ezUInt32 i = 0; i < uiNumAnimations; ++i)
  {
    stream << m_Animations[i].m_sPropertyName;
    stream << m_Animations[i].m_Duration;
    stream << m_Animations[i].m_Mode.GetValue();
    stream << m_Animations[i].m_Target.GetValue();
    stream << m_Animations[i].m_hNumberCurve;
    stream << m_Animations[i].m_hColorCurve;
  }
}

void ezPropertyAnimResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  ezUInt8 uiIdentifier = 0;
  ezUInt16 uiNumAnimations = 0;

  stream >> uiVersion;
  stream >> uiIdentifier;
  stream >> uiNumAnimations;

  EZ_ASSERT_DEV(uiIdentifier == 0x0A, "File does not contain a valid ezPropertyAnimResourceDescriptor");
  EZ_ASSERT_DEV(uiVersion == 1, "Invalid file version %u", uiVersion);

  m_Animations.SetCount(uiNumAnimations);

  for (ezUInt32 i = 0; i < uiNumAnimations; ++i)
  {
    stream >> m_Animations[i].m_sPropertyName;
    stream >> m_Animations[i].m_Duration;

    ezPropertyAnimMode::StorageType mode;
    stream >> mode;  m_Animations[i].m_Mode.SetValue(mode);

    ezPropertyAnimTarget::StorageType target;
    stream >> target;  m_Animations[i].m_Target.SetValue(target);

    stream >> m_Animations[i].m_hNumberCurve;
    stream >> m_Animations[i].m_hColorCurve;
  }
}


void ezPropertyAnimEntry::SetNumberCurveFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hNumberCurve = ezResourceManager::LoadResource<ezCurve1DResource>(szFile);
  }
}

const char* ezPropertyAnimEntry::GetNumberCurveFile() const
{
  if (m_hNumberCurve.IsValid())
    return m_hNumberCurve.GetResourceID();

  return "";
}

void ezPropertyAnimEntry::SetColorCurveFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hColorCurve = ezResourceManager::LoadResource<ezColorGradientResource>(szFile);
  }
}

const char* ezPropertyAnimEntry::GetColorCurveFile() const
{
  if (m_hColorCurve.IsValid())
    return m_hColorCurve.GetResourceID();

  return "";
}
