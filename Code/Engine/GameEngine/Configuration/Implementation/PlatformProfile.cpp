#include <PCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GameEngine/Configuration/PlatformProfile.h>
#include <Foundation/Reflection/ReflectionUtils.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProfileTargetPlatform, 1)
  EZ_ENUM_CONSTANTS(ezProfileTargetPlatform::PC, ezProfileTargetPlatform::UWP, ezProfileTargetPlatform::Android)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProfileConfigData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

ezProfileConfigData::ezProfileConfigData() = default;
ezProfileConfigData::~ezProfileConfigData() = default;

void ezProfileConfigData::SaveRuntimeData(ezChunkStreamWriter& stream) const {}
void ezProfileConfigData::LoadRuntimeData(ezChunkStreamReader& stream) {}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPlatformProfile, 1, ezRTTIDefaultAllocator<ezPlatformProfile>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new ezHiddenAttribute()),
    EZ_ENUM_MEMBER_PROPERTY("Platform", ezProfileTargetPlatform, m_TargetPlatform),
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
  for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    // find all types derived from ezProfileConfigData
    if (!pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract) && pRtti->IsDerivedFrom<ezProfileConfigData>() &&
        pRtti->GetAllocator()->CanAllocate())
    {
      bool bHasTypeAlready = false;

      // check whether we already have an instance of this type
      for (auto pType : m_Configs)
      {
        if (pType->GetDynamicRTTI() == pRtti)
        {
          bHasTypeAlready = true;
          break;
        }
      }

      if (!bHasTypeAlready)
      {
        // if not, allocate one
        ezProfileConfigData* pObject = pRtti->GetAllocator()->Allocate<ezProfileConfigData>();
        ezReflectionUtils::SetAllMemberPropertiesToDefault(pRtti, pObject);

        m_Configs.PushBack(pObject);
      }
    }
  }

  // sort all configs alphabetically
  m_Configs.Sort([](const ezProfileConfigData* lhs, const ezProfileConfigData* rhs) -> bool {
    return ezStringUtils::Compare(lhs->GetDynamicRTTI()->GetTypeName(), rhs->GetDynamicRTTI()->GetTypeName()) < 0;
  });
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

ezResult ezPlatformProfile::SaveForRuntime(const char* szFile) const
{
  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  ezChunkStreamWriter chunk(file);

  chunk.BeginStream(1);

  for (auto* pConfig : m_Configs)
  {
    pConfig->SaveRuntimeData(chunk);
  }

  chunk.EndStream();

  return EZ_SUCCESS;
}

ezResult ezPlatformProfile::LoadForRuntime(const char* szFile)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

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

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineProfileConfig, 1, ezRTTIDefaultAllocator<ezRenderPipelineProfileConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    // MainRenderPipeline.ezRenderPipelineAsset
    EZ_MEMBER_PROPERTY("MainRenderPipeline", m_sMainRenderPipeline)->AddAttributes(new ezAssetBrowserAttribute("RenderPipeline"), new ezDefaultValueAttribute(ezStringView("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }"))),
    // EditorRenderPipeline.ezRenderPipelineAsset
    //EZ_MEMBER_PROPERTY("EditorRenderPipeline", m_sEditorRenderPipeline)->AddAttributes(new ezAssetBrowserAttribute("RenderPipeline"), new ezDefaultValueAttribute(ezStringView("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"))),
    // DebugRenderPipeline.ezRenderPipelineAsset
    //EZ_MEMBER_PROPERTY("DebugRenderPipeline", m_sDebugRenderPipeline)->AddAttributes(new ezAssetBrowserAttribute("RenderPipeline"), new ezDefaultValueAttribute(ezStringView("{ 0416eb3e-69c0-4640-be5b-77354e0e37d7 }"))),

    EZ_MAP_MEMBER_PROPERTY("CameraPipelines", m_CameraPipelines)->AddAttributes(new ezAssetBrowserAttribute("RenderPipeline")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezRenderPipelineProfileConfig::SaveRuntimeData(ezChunkStreamWriter& stream) const
{
  stream.BeginChunk("ezRenderPipelineProfileConfig", 2);

  stream << m_sMainRenderPipeline;

  stream << m_CameraPipelines.GetCount();
  for (auto it = m_CameraPipelines.GetIterator(); it.IsValid(); ++it)
  {
    stream << it.Key();
    stream << it.Value();
  }

  stream.EndChunk();
}

void ezRenderPipelineProfileConfig::LoadRuntimeData(ezChunkStreamReader& stream)
{
  const auto& chunk = stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "ezRenderPipelineProfileConfig" && chunk.m_uiChunkVersion == 2)
  {
    stream >> m_sMainRenderPipeline;

    m_CameraPipelines.Clear();

    ezUInt32 uiNumCamPipes = 0;
    stream >> uiNumCamPipes;
    for (ezUInt32 i = 0; i < uiNumCamPipes; ++i)
    {
      ezString sPipeName, sPipeAsset;

      stream >> sPipeName;
      stream >> sPipeAsset;

      m_CameraPipelines[sPipeName] = sPipeAsset;
    }
  }
}
