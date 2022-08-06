#include <Core/CorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSurfaceInteractionAlignment, 2)
  EZ_ENUM_CONSTANTS(ezSurfaceInteractionAlignment::SurfaceNormal, ezSurfaceInteractionAlignment::IncidentDirection, ezSurfaceInteractionAlignment::ReflectedDirection)
  EZ_ENUM_CONSTANTS(ezSurfaceInteractionAlignment::ReverseSurfaceNormal, ezSurfaceInteractionAlignment::ReverseIncidentDirection, ezSurfaceInteractionAlignment::ReverseReflectedDirection)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezSurfaceInteraction, ezNoBase, 1, ezRTTIDefaultAllocator<ezSurfaceInteraction>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Type", m_sInteractionType)->AddAttributes(new ezDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    EZ_ACCESSOR_PROPERTY("Prefab", GetPrefab, SetPrefab)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    // this does not work yet (asset transform fails)
    //EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Prefab")),
    EZ_ENUM_MEMBER_PROPERTY("Alignment", ezSurfaceInteractionAlignment, m_Alignment),
    EZ_MEMBER_PROPERTY("Deviation", m_Deviation)->AddAttributes(new ezClampValueAttribute(ezVariant(ezAngle::Degree(0.0f)), ezVariant(ezAngle::Degree(90.0f)))),
    EZ_MEMBER_PROPERTY("ImpulseThreshold", m_fImpulseThreshold),
    EZ_MEMBER_PROPERTY("ImpulseScale", m_fImpulseScale)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceResourceDescriptor, 3, ezRTTIDefaultAllocator<ezSurfaceResourceDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BaseSurface", GetBaseSurfaceFile, SetBaseSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
    EZ_MEMBER_PROPERTY("Restitution", m_fPhysicsRestitution)->AddAttributes(new ezDefaultValueAttribute(0.25f)),
    EZ_MEMBER_PROPERTY("StaticFriction", m_fPhysicsFrictionStatic)->AddAttributes(new ezDefaultValueAttribute(0.6f)),
    EZ_MEMBER_PROPERTY("DynamicFriction", m_fPhysicsFrictionDynamic)->AddAttributes(new ezDefaultValueAttribute(0.4f)),
    EZ_MEMBER_PROPERTY("SoundObstruction", m_fSoundObstruction)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ACCESSOR_PROPERTY("OnCollideInteraction", GetCollisionInteraction, SetCollisionInteraction)->AddAttributes(new ezDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    EZ_ACCESSOR_PROPERTY("SlideReaction", GetSlideReactionPrefabFile, SetSlideReactionPrefabFile)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_ACCESSOR_PROPERTY("RollReaction", GetRollReactionPrefabFile, SetRollReactionPrefabFile)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
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

const ezRangeView<const char*, ezUInt32> ezSurfaceInteraction::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32 { return 0; },
    [this]() -> ezUInt32 { return m_Parameters.GetCount(); },
    [](ezUInt32& it) { ++it; },
    [this](const ezUInt32& it) -> const char* { return m_Parameters.GetKey(it).GetString().GetData(); });
}

void ezSurfaceInteraction::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != ezInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void ezSurfaceInteraction::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(ezTempHashedString(szKey));
}

bool ezSurfaceInteraction::GetParameter(const char* szKey, ezVariant& out_value) const
{
  ezUInt32 it = m_Parameters.Find(szKey);

  if (it == ezInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

void ezSurfaceResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= 8, "Invalid version {0} for surface resource", uiVersion);

  stream >> m_fPhysicsRestitution;
  stream >> m_fPhysicsFrictionStatic;
  stream >> m_fPhysicsFrictionDynamic;
  stream >> m_hBaseSurface;

  if (uiVersion >= 8)
  {
    stream >> m_fSoundObstruction;
  }

  if (uiVersion >= 4)
  {
    stream >> m_sOnCollideInteraction;
  }

  if (uiVersion >= 7)
  {
    stream >> m_sSlideInteractionPrefab;
    stream >> m_sRollInteractionPrefab;
  }

  if (uiVersion > 2)
  {
    ezUInt32 count = 0;
    stream >> count;
    m_Interactions.SetCount(count);

    ezStringBuilder sTemp;
    for (ezUInt32 i = 0; i < count; ++i)
    {
      auto& ia = m_Interactions[i];

      stream >> sTemp;
      ia.m_sInteractionType = sTemp;

      stream >> ia.m_hPrefab;
      stream >> ia.m_Alignment;
      stream >> ia.m_Deviation;

      if (uiVersion >= 4)
      {
        stream >> ia.m_fImpulseThreshold;
      }

      if (uiVersion >= 5)
      {
        stream >> ia.m_fImpulseScale;
      }

      if (uiVersion >= 6)
      {
        ezUInt8 uiNumParams;
        stream >> uiNumParams;

        ia.m_Parameters.Clear();
        ia.m_Parameters.Reserve(uiNumParams);

        ezHashedString key;
        ezVariant value;

        for (ezUInt32 i2 = 0; i2 < uiNumParams; ++i2)
        {
          stream >> key;
          stream >> value;

          ia.m_Parameters.Insert(key, value);
        }
      }
    }
  }
}

void ezSurfaceResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 8;

  stream << uiVersion;
  stream << m_fPhysicsRestitution;
  stream << m_fPhysicsFrictionStatic;
  stream << m_fPhysicsFrictionDynamic;
  stream << m_hBaseSurface;

  // version 8
  stream << m_fSoundObstruction;

  // version 4
  stream << m_sOnCollideInteraction;

  // version 7
  stream << m_sSlideInteractionPrefab;
  stream << m_sRollInteractionPrefab;

  stream << m_Interactions.GetCount();
  for (const auto& ia : m_Interactions)
  {
    stream << ia.m_sInteractionType;
    stream << ia.m_hPrefab;
    stream << ia.m_Alignment;
    stream << ia.m_Deviation;

    // version 4
    stream << ia.m_fImpulseThreshold;

    // version 5
    stream << ia.m_fImpulseScale;

    // version 6
    const ezUInt8 uiNumParams = static_cast<ezUInt8>(ia.m_Parameters.GetCount());
    stream << uiNumParams;
    for (ezUInt32 i = 0; i < uiNumParams; ++i)
    {
      stream << ia.m_Parameters.GetKey(i);
      stream << ia.m_Parameters.GetValue(i);
    }
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

void ezSurfaceResourceDescriptor::SetCollisionInteraction(const char* name)
{
  m_sOnCollideInteraction.Assign(name);
}

const char* ezSurfaceResourceDescriptor::GetCollisionInteraction() const
{
  return m_sOnCollideInteraction.GetData();
}

void ezSurfaceResourceDescriptor::SetSlideReactionPrefabFile(const char* szFile)
{
  m_sSlideInteractionPrefab.Assign(szFile);
}

const char* ezSurfaceResourceDescriptor::GetSlideReactionPrefabFile() const
{
  return m_sSlideInteractionPrefab.GetData();
}

void ezSurfaceResourceDescriptor::SetRollReactionPrefabFile(const char* szFile)
{
  m_sRollInteractionPrefab.Assign(szFile);
}

const char* ezSurfaceResourceDescriptor::GetRollReactionPrefabFile() const
{
  return m_sRollInteractionPrefab.GetData();
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
