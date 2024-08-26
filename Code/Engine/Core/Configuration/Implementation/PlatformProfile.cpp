#include <Core/CorePCH.h>

#include <Core/Configuration/PlatformProfile.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>

#include <Core/ResourceManager/ResourceManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProfileConfigData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

ezProfileConfigData::ezProfileConfigData() = default;
ezProfileConfigData::~ezProfileConfigData() = default;

void ezProfileConfigData::SaveRuntimeData(ezChunkStreamWriter& inout_stream) const
{
  EZ_IGNORE_UNUSED(inout_stream);
}

void ezProfileConfigData::LoadRuntimeData(ezChunkStreamReader& inout_stream)
{
  EZ_IGNORE_UNUSED(inout_stream);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPlatformProfile, 1, ezRTTIDefaultAllocator<ezPlatformProfile>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("TargetPlatform", m_sTargetPlatform)->AddAttributes(new ezDynamicStringEnumAttribute("TargetPlatformNames"), new ezDefaultValueAttribute("Windows")),
    EZ_ARRAY_MEMBER_PROPERTY("Configs", m_Configs)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezContainerAttribute(false, false, false)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPlatformProfile::ezPlatformProfile() = default;

ezPlatformProfile::~ezPlatformProfile()
{
  Clear();
}

void ezPlatformProfile::Clear()
{
  for (auto pType : m_Configs)
  {
    pType->GetDynamicRTTI()->GetAllocator()->Deallocate(pType);
  }

  m_Configs.Clear();
}

void ezPlatformProfile::AddMissingConfigs()
{
  ezRTTI::ForEachDerivedType<ezProfileConfigData>(
    [this](const ezRTTI* pRtti)
    {
      // find all types derived from ezProfileConfigData
      bool bHasTypeAlready = false;

      // check whether we already have an instance of this type
      for (auto pType : m_Configs)
      {
        if (pType && pType->GetDynamicRTTI() == pRtti)
        {
          bHasTypeAlready = true;
          break;
        }
      }

      if (!bHasTypeAlready)
      {
        // if not, allocate one
        ezProfileConfigData* pObject = pRtti->GetAllocator()->Allocate<ezProfileConfigData>();
        EZ_ASSERT_DEV(pObject != nullptr, "Invalid profile config");
        ezReflectionUtils::SetAllMemberPropertiesToDefault(pRtti, pObject);

        m_Configs.PushBack(pObject);
      }
    },
    ezRTTI::ForEachOptions::ExcludeNonAllocatable);

  // in case unknown configs were loaded from disk, remove them
  m_Configs.RemoveAndSwap(nullptr);

  // sort all configs alphabetically
  m_Configs.Sort([](const ezProfileConfigData* lhs, const ezProfileConfigData* rhs) -> bool
    { return lhs->GetDynamicRTTI()->GetTypeName().Compare(rhs->GetDynamicRTTI()->GetTypeName()) < 0; });
}

const ezProfileConfigData* ezPlatformProfile::GetTypeConfig(const ezRTTI* pRtti) const
{
  for (const auto* pConfig : m_Configs)
  {
    if (pConfig->GetDynamicRTTI() == pRtti)
      return pConfig;
  }

  return nullptr;
}

ezProfileConfigData* ezPlatformProfile::GetTypeConfig(const ezRTTI* pRtti)
{
  // reuse the const-version
  return const_cast<ezProfileConfigData*>(((const ezPlatformProfile*)this)->GetTypeConfig(pRtti));
}

ezResult ezPlatformProfile::SaveForRuntime(ezStringView sFile) const
{
  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile));

  ezChunkStreamWriter chunk(file);

  chunk.BeginStream(1);

  for (auto* pConfig : m_Configs)
  {
    pConfig->SaveRuntimeData(chunk);
  }

  chunk.EndStream();

  return EZ_SUCCESS;
}

ezResult ezPlatformProfile::LoadForRuntime(ezStringView sFile)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile));

  ezChunkStreamReader chunk(file);

  chunk.BeginStream();

  while (chunk.GetCurrentChunk().m_bValid)
  {
    for (auto* pConfig : m_Configs)
    {
      pConfig->LoadRuntimeData(chunk);
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  ++m_uiLastModificationCounter;
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(Core, Core_Configuration_Implementation_PlatformProfile);
