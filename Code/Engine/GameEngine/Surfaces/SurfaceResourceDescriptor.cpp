#include <PCH.h>

#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <GameEngine/Surfaces/SurfaceResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSurfaceInteractionAlignment, 2)
  EZ_ENUM_CONSTANTS(ezSurfaceInteractionAlignment::SurfaceNormal, ezSurfaceInteractionAlignment::IncidentDirection, ezSurfaceInteractionAlignment::ReflectedDirection)
  EZ_ENUM_CONSTANTS(ezSurfaceInteractionAlignment::ReverseSurfaceNormal, ezSurfaceInteractionAlignment::ReverseIncidentDirection, ezSurfaceInteractionAlignment::ReverseReflectedDirection)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezSurfaceInteraction, ezNoBase, 1, ezRTTIDefaultAllocator<ezSurfaceInteraction>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Type", m_sInteractionType),
    EZ_ACCESSOR_PROPERTY("Prefab", GetPrefab, SetPrefab)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_ENUM_MEMBER_PROPERTY("Alignment", ezSurfaceInteractionAlignment, m_Alignment),
    EZ_MEMBER_PROPERTY("Deviation", m_Deviation)->AddAttributes(new ezClampValueAttribute(ezVariant(ezAngle::Degree(0.0f)), ezVariant(ezAngle::Degree(90.0f)))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceResourceDescriptor, 2, ezRTTIDefaultAllocator<ezSurfaceResourceDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BaseSurface", GetBaseSurfaceFile, SetBaseSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
    EZ_MEMBER_PROPERTY("Restitution", m_fPhysicsRestitution)->AddAttributes(new ezDefaultValueAttribute(0.25f)),
    EZ_MEMBER_PROPERTY("StaticFriction", m_fPhysicsFrictionStatic)->AddAttributes(new ezDefaultValueAttribute(0.6f)),
    EZ_MEMBER_PROPERTY("DynamicFriction", m_fPhysicsFrictionDynamic)->AddAttributes(new ezDefaultValueAttribute(0.4f)),
    EZ_ARRAY_MEMBER_PROPERTY("Interactions", m_Interactions),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezSurfaceInteraction::SetPrefab(const char* szPrefab)
{
  ezPrefabResourceHandle hPrefab;

  if (!ezStringUtils::IsNullOrEmpty(szPrefab))
  {
    hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(szPrefab);
  }

  m_hPrefab = hPrefab;
}

const char* ezSurfaceInteraction::GetPrefab() const
{
  if (!m_hPrefab.IsValid())
    return "";

  return m_hPrefab.GetResourceID();
}



void ezSurfaceResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= 3, "Invalid version {0} for surface resource", uiVersion);

  stream >> m_fPhysicsRestitution;
  stream >> m_fPhysicsFrictionStatic;
  stream >> m_fPhysicsFrictionDynamic;
  stream >> m_hBaseSurface;

  if (uiVersion > 2)
  {
    ezUInt32 count = 0;
    stream >> count;
    m_Interactions.SetCount(count);

    ezStringBuilder sTemp;
    for (ezUInt32 i = 0; i < count; ++i)
    {
      stream >> sTemp;
      m_Interactions[i].m_sInteractionType = sTemp;

      stream >> m_Interactions[i].m_hPrefab;
      stream >> m_Interactions[i].m_Alignment;
      stream >> m_Interactions[i].m_Deviation;
    }
  }
}

void ezSurfaceResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 3;

  stream << uiVersion;
  stream << m_fPhysicsRestitution;
  stream << m_fPhysicsFrictionStatic;
  stream << m_fPhysicsFrictionDynamic;
  stream << m_hBaseSurface;

  stream << m_Interactions.GetCount();
  for (const auto& ia : m_Interactions)
  {
    stream << ia.m_sInteractionType;
    stream << ia.m_hPrefab;
    stream << ia.m_Alignment;
    stream << ia.m_Deviation;
  }
}

void ezSurfaceResourceDescriptor::SetBaseSurfaceFile(const char* szFile)
{
  ezSurfaceResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezSurfaceResource>(szFile);
  }

  m_hBaseSurface = hResource;
}

const char* ezSurfaceResourceDescriptor::GetBaseSurfaceFile() const
{
  if (!m_hBaseSurface.IsValid())
    return "";

  return m_hBaseSurface.GetResourceID();
}

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezSurfaceResourceDescriptorPatch_1_2 : public ezGraphPatch
{
public:
  ezSurfaceResourceDescriptorPatch_1_2()
      : ezGraphPatch("ezSurfaceResourceDescriptor", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Base Surface", "BaseSurface");
    pNode->RenameProperty("Static Friction", "StaticFriction");
    pNode->RenameProperty("Dynamic Friction", "DynamicFriction");
  }
};

ezSurfaceResourceDescriptorPatch_1_2 g_ezSurfaceResourceDescriptorPatch_1_2;



EZ_STATICLINK_FILE(GameEngine, GameEngine_Surfaces_SurfaceResourceDescriptor);

