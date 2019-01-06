#include <PCH.h>
#include <EditorFramework/GUI/ExposedParametersTypeRegistry.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <Assets/AssetCurator.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

EZ_IMPLEMENT_SINGLETON(ezExposedParametersTypeRegistry);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, ExposedParametersTypeRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager", "AssetCurator"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezExposedParametersTypeRegistry);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezExposedParametersTypeRegistry* pDummy = ezExposedParametersTypeRegistry::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezExposedParametersTypeRegistry::ezExposedParametersTypeRegistry()
    : m_SingletonRegistrar(this)
{
  ezReflectedTypeDescriptor desc;
  desc.m_sTypeName = "ezExposedParametersTypeBase";
  desc.m_sPluginName = "ExposedParametersTypes";
  desc.m_sParentTypeName = ezGetStaticRTTI<ezReflectedClass>()->GetTypeName();
  desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Abstract | ezTypeFlags::Class;
  desc.m_uiTypeSize = 0;
  desc.m_uiTypeVersion = 0;

  m_pBaseType = ezPhantomRttiManager::RegisterType(desc);

  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezExposedParametersTypeRegistry::AssetCuratorEventHandler, this));
  ezPhantomRttiManager::s_Events.AddEventHandler(ezMakeDelegate(&ezExposedParametersTypeRegistry::PhantomTypeRegistryEventHandler, this));
}


ezExposedParametersTypeRegistry::~ezExposedParametersTypeRegistry()
{
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezExposedParametersTypeRegistry::AssetCuratorEventHandler, this));
  ezPhantomRttiManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezExposedParametersTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

const ezRTTI* ezExposedParametersTypeRegistry::GetExposedParametersType(const char* szResource)
{
  if (ezStringUtils::IsNullOrEmpty(szResource))
    return nullptr;

  const auto asset = ezAssetCurator::GetSingleton()->FindSubAsset(szResource);
  if (!asset)
    return nullptr;

  auto params = asset->m_pAssetInfo->m_Info->GetMetaInfo<ezExposedParameters>();
  if (!params)
    return nullptr;

  auto it = m_ShaderTypes.Find(asset->m_Data.m_Guid);
  if (it.IsValid())
  {
    if (!it.Value().m_bUpToDate)
    {
      UpdateExposedParametersType(it.Value(), *params);
    }
  }
  else
  {
    it = m_ShaderTypes.Insert(asset->m_Data.m_Guid, ParamData());
    it.Value().m_SubAssetGuid = asset->m_Data.m_Guid;
    UpdateExposedParametersType(it.Value(), *params);
  }

  return it.Value().m_pType;
}

void ezExposedParametersTypeRegistry::UpdateExposedParametersType(ParamData& data, const ezExposedParameters& params)
{
  ezStringBuilder name;
  name.Format("ezExposedParameters_{0}", data.m_SubAssetGuid);
  EZ_LOG_BLOCK("Updating Type", name.GetData());
  ezReflectedTypeDescriptor desc;
  desc.m_sTypeName = name;
  desc.m_sPluginName = "ExposedParametersTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Class;
  desc.m_uiTypeSize = 0;
  desc.m_uiTypeVersion = 2;

  for (const auto* parameter : params.m_Parameters)
  {
    const ezRTTI* pType = ezReflectionUtils::GetTypeFromVariant(parameter->m_DefaultValue);
    if (!parameter->m_sType.IsEmpty())
    {
      if (const ezRTTI* pType2 = ezRTTI::FindTypeByName(parameter->m_sType))
        pType = pType2;
    }
    if (pType == nullptr)
      continue;

    ezBitflags<ezPropertyFlags> flags = ezPropertyFlags::Phantom;
    if (pType->IsDerivedFrom<ezEnumBase>())
      flags |= ezPropertyFlags::IsEnum;
    if (pType->IsDerivedFrom<ezBitflagsBase>())
      flags |= ezPropertyFlags::Bitflags;
    if (ezReflectionUtils::IsBasicType(pType))
      flags |= ezPropertyFlags::StandardType;

    ezReflectedPropertyDescriptor propDesc(ezPropertyCategory::Member, parameter->m_sName, pType->GetTypeName(), flags);
    for (auto attrib : parameter->m_Attributes)
    {
      propDesc.m_Attributes.PushBack(ezReflectionSerializer::Clone(attrib));
    }
    desc.m_Properties.PushBack(propDesc);
  }

  // Register and return the phantom type. If the type already exists this will update the type
  // and patch any existing instances of it so they should show up in the prop grid right away.
  {
    // This fkt is called by the property grid, but calling RegisterType will update the property grid
    // and we will recurse into this. So we listen for the ezPhantomRttiManager events to fill out
    // the data.m_pType in it to make sure recursion into GetExposedParametersType does not return a nullptr.
    m_pAboutToBeRegistered = &data;
    data.m_bUpToDate = true;
    data.m_pType = ezPhantomRttiManager::RegisterType(desc);
    m_pAboutToBeRegistered = nullptr;
  }
}

void ezExposedParametersTypeRegistry::AssetCuratorEventHandler(const ezAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
  case ezAssetCuratorEvent::Type::AssetRemoved:
    {
      // Ignore for now, doesn't hurt. Removing types is more hassle than it is worth.
      if (auto* data = m_ShaderTypes.GetValue(e.m_AssetGuid))
      {
        data->m_bUpToDate = false;
      }
    }
    break;
  case ezAssetCuratorEvent::Type::AssetListReset:
    {
      for (auto it = m_ShaderTypes.GetIterator(); it.IsValid(); ++it)
      {
        it.Value().m_bUpToDate = false;
      }
    }
    break;
  case ezAssetCuratorEvent::Type::AssetUpdated:
    {
      if (auto* data = m_ShaderTypes.GetValue(e.m_AssetGuid))
      {
        data->m_bUpToDate = false;
        if (auto params = e.m_pInfo->m_pAssetInfo->m_Info->GetMetaInfo<ezExposedParameters>())
          UpdateExposedParametersType(*data, *params);
      }
    }
    break;
  }
}

void ezExposedParametersTypeRegistry::PhantomTypeRegistryEventHandler(const ezPhantomRttiManagerEvent& e)
{
  if (e.m_Type == ezPhantomRttiManagerEvent::Type::TypeAdded || e.m_Type == ezPhantomRttiManagerEvent::Type::TypeChanged)
  {
    if (e.m_pChangedType->GetParentType() == m_pBaseType && m_pAboutToBeRegistered)
    {
      // We listen for the ezPhantomRttiManager events to fill out the m_pType pointer. This is needed as otherwise
      // Recursion into GetExposedParametersType would return a nullptr.
      m_pAboutToBeRegistered->m_pType = e.m_pChangedType;
    }
  }
}
