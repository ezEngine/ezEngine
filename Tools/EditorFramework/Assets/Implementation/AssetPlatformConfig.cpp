#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetPlatformConfig.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/IO/FileSystem/FileReader.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetTypePlatformConfig, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezAssetTargetPlatform, 1)
  EZ_ENUM_CONSTANTS(ezAssetTargetPlatform::PC, ezAssetTargetPlatform::Android)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetPlatformConfig, 1, ezRTTIDefaultAllocator<ezAssetPlatformConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new ezHiddenAttribute()),
    EZ_ENUM_MEMBER_PROPERTY("Platform", ezAssetTargetPlatform, m_TargetPlatform),
    EZ_ARRAY_MEMBER_PROPERTY("TypeConfigs", m_TypeConfigs)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezContainerAttribute(false, false, false)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

ezAssetTypePlatformConfig::ezAssetTypePlatformConfig() = default;
ezAssetTypePlatformConfig::~ezAssetTypePlatformConfig() = default;

ezAssetPlatformConfig::ezAssetPlatformConfig()
{
  InitializeToDefault();
}

ezAssetPlatformConfig::~ezAssetPlatformConfig()
{
  Clear();
}

void ezAssetPlatformConfig::InitializeToDefault()
{
  Clear();

  m_TargetPlatform = ezAssetTargetPlatform::Default;

  for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract) && pRtti->IsDerivedFrom<ezAssetTypePlatformConfig>() &&
        pRtti->GetAllocator()->CanAllocate())
    {
      m_TypeConfigs.PushBack(pRtti->GetAllocator()->Allocate<ezAssetTypePlatformConfig>());
    }
  }

  m_TypeConfigs.Sort([](const ezAssetTypePlatformConfig* lhs, const ezAssetTypePlatformConfig* rhs) -> bool {
    return ezStringUtils::Compare(lhs->GetDisplayName(), rhs->GetDisplayName()) < 0;
  });
}

void ezAssetPlatformConfig::Clear()
{
  for (auto pType : m_TypeConfigs)
  {
    pType->GetDynamicRTTI()->GetAllocator()->Deallocate(pType);
  }

  m_TypeConfigs.Clear();
}


const ezAssetTypePlatformConfig* ezAssetPlatformConfig::GetTypeConfig(const ezRTTI* pRtti) const
{
  for (const auto* pConfig : m_TypeConfigs)
  {
    if (pConfig->GetDynamicRTTI() == pRtti)
      return pConfig;
  }

  return nullptr;
}

ezAssetTypePlatformConfig* ezAssetPlatformConfig::GetTypeConfig(const ezRTTI* pRtti)
{
  for (auto* pConfig : m_TypeConfigs)
  {
    if (pConfig->GetDynamicRTTI() == pRtti)
      return pConfig;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetTypePlatformConfig, 1, ezRTTIDefaultAllocator<ezTextureAssetTypePlatformConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MaxResolution", m_uiMaxResolution)->AddAttributes(new ezDefaultValueAttribute(16 * 1024)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const char* ezTextureAssetTypePlatformConfig::GetDisplayName() const
{
  return "2D Textures";
}

//////////////////////////////////////////////////////////////////////////


ezResult ezAssetCurator::SaveAssetPlatformConfigs()
{
  ezDeferredFileWriter file;
  file.SetOutput(":project/Editor/AssetConfigs.ddl");

  ezOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  ddl.BeginObject("AssetConfigs");

  for (const auto* pCfg : m_AssetPlatformConfigs)
  {
    ddl.BeginObject("Config", pCfg->m_sName);

    // make sure to create the same GUID every time, otherwise the serialized file changes all the time
    const ezUuid guid = ezUuid::StableUuidForString(pCfg->GetConfigName());

    ezReflectionSerializer::WriteObjectToDDL(ddl, pCfg->GetDynamicRTTI(), pCfg, guid);

    ddl.EndObject();
  }

  ddl.EndObject();

  return file.Close();
}

ezResult ezAssetCurator::LoadAssetPlatformConfigs()
{
  EZ_LOG_BLOCK("LoadAssetConfigs", ":project/Editor/AssetConfigs.ddl");

  ezFileReader file;
  if (file.Open(":project/Editor/AssetConfigs.ddl").Failed())
  {
    ezLog::Warning("Asset configurations file does not exist.");
    return EZ_FAILURE;
  }

  ezOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
  {
    ezLog::Error("Asset config file could not be parsed.");
    return EZ_FAILURE;
  }

  const ezOpenDdlReaderElement* pRootElement = ddl.GetRootElement()->FindChildOfType("AssetConfigs");

  if (!pRootElement)
    return EZ_FAILURE;

  ClearAssetPlatformConfigs();

  for (auto pChild = pRootElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Config"))
    {
      const ezRTTI* pRtti = nullptr;
      void* pConfigObj = ezReflectionSerializer::ReadObjectFromDDL(pChild, pRtti);

      m_AssetPlatformConfigs.PushBack(static_cast<ezAssetPlatformConfig*>(pConfigObj));
    }
  }

  return EZ_SUCCESS;
}

void ezAssetCurator::ClearAssetPlatformConfigs()
{
  for (auto pCfg : m_AssetPlatformConfigs)
  {
    pCfg->GetDynamicRTTI()->GetAllocator()->Deallocate(pCfg);
  }

  m_AssetPlatformConfigs.Clear();
}

void ezAssetCurator::SetupDefaultAssetPlatformConfigs()
{
  ClearAssetPlatformConfigs();

  {
    ezAssetPlatformConfig* pCfg = EZ_DEFAULT_NEW(ezAssetPlatformConfig);
    pCfg->m_sName = "PC";
    m_AssetPlatformConfigs.PushBack(pCfg);
  }

  {
    ezAssetPlatformConfig* pCfg = EZ_DEFAULT_NEW(ezAssetPlatformConfig);
    pCfg->m_sName = "PC-low";
    pCfg->GetTypeConfig<ezTextureAssetTypePlatformConfig>()->m_uiMaxResolution = 256;
    m_AssetPlatformConfigs.PushBack(pCfg);
  }

  {
    ezAssetPlatformConfig* pCfg = EZ_DEFAULT_NEW(ezAssetPlatformConfig);
    pCfg->m_sName = "Android";
    pCfg->m_TargetPlatform = ezAssetTargetPlatform::Android;
    pCfg->GetTypeConfig<ezTextureAssetTypePlatformConfig>()->m_uiMaxResolution = 512;
    m_AssetPlatformConfigs.PushBack(pCfg);
  }
}
