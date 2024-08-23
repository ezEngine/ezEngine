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
    EZ_RESOURCE_MEMBER_PROPERTY("Prefab", m_hPrefab)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Prefab")),
    EZ_ENUM_MEMBER_PROPERTY("Alignment", ezSurfaceInteractionAlignment, m_Alignment),
    EZ_MEMBER_PROPERTY("Deviation", m_Deviation)->AddAttributes(new ezClampValueAttribute(ezVariant(ezAngle::MakeFromDegree(0.0f)), ezVariant(ezAngle::MakeFromDegree(90.0f)))),
    EZ_MEMBER_PROPERTY("ImpulseThreshold", m_fImpulseThreshold),
    EZ_MEMBER_PROPERTY("ImpulseScale", m_fImpulseScale)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceResourceDescriptor, 2, ezRTTIDefaultAllocator<ezSurfaceResourceDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("BaseSurface", m_hBaseSurface)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface")),// package+thumbnail so that it forbids circular dependencies
    EZ_MEMBER_PROPERTY("Restitution", m_fPhysicsRestitution)->AddAttributes(new ezDefaultValueAttribute(0.25f)),
    EZ_MEMBER_PROPERTY("StaticFriction", m_fPhysicsFrictionStatic)->AddAttributes(new ezDefaultValueAttribute(0.6f)),
    EZ_MEMBER_PROPERTY("DynamicFriction", m_fPhysicsFrictionDynamic)->AddAttributes(new ezDefaultValueAttribute(0.4f)),
    EZ_MEMBER_PROPERTY("GroundType", m_iGroundType)->AddAttributes(new ezDefaultValueAttribute(-1), new ezDynamicEnumAttribute("AiGroundType")),
    EZ_ACCESSOR_PROPERTY("OnCollideInteraction", GetCollisionInteraction, SetCollisionInteraction)->AddAttributes(new ezDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    EZ_ACCESSOR_PROPERTY("SlideReaction", GetSlideReactionPrefabFile, SetSlideReactionPrefabFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
    EZ_ACCESSOR_PROPERTY("RollReaction", GetRollReactionPrefabFile, SetRollReactionPrefabFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
    EZ_ARRAY_MEMBER_PROPERTY("Interactions", m_Interactions),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRangeView<const char*, ezUInt32> ezSurfaceInteraction::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32
    { return 0; },
    [this]() -> ezUInt32
    { return m_Parameters.GetCount(); },
    [](ezUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const ezUInt32& uiIt) -> const char*
    { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
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

void ezSurfaceResourceDescriptor::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;

  inout_stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= 8, "Invalid version {0} for surface resource", uiVersion);

  inout_stream >> m_fPhysicsRestitution;
  inout_stream >> m_fPhysicsFrictionStatic;
  inout_stream >> m_fPhysicsFrictionDynamic;
  inout_stream >> m_hBaseSurface;

  if (uiVersion >= 4)
  {
    inout_stream >> m_sOnCollideInteraction;
  }

  if (uiVersion >= 7)
  {
    inout_stream >> m_sSlideInteractionPrefab;
    inout_stream >> m_sRollInteractionPrefab;
  }

  if (uiVersion > 2)
  {
    ezUInt32 count = 0;
    inout_stream >> count;
    m_Interactions.SetCount(count);

    ezStringBuilder sTemp;
    for (ezUInt32 i = 0; i < count; ++i)
    {
      auto& ia = m_Interactions[i];

      inout_stream >> sTemp;
      ia.m_sInteractionType = sTemp;

      inout_stream >> ia.m_hPrefab;
      inout_stream >> ia.m_Alignment;
      inout_stream >> ia.m_Deviation;

      if (uiVersion >= 4)
      {
        inout_stream >> ia.m_fImpulseThreshold;
      }

      if (uiVersion >= 5)
      {
        inout_stream >> ia.m_fImpulseScale;
      }

      if (uiVersion >= 6)
      {
        ezUInt8 uiNumParams;
        inout_stream >> uiNumParams;

        ia.m_Parameters.Clear();
        ia.m_Parameters.Reserve(uiNumParams);

        ezHashedString key;
        ezVariant value;

        for (ezUInt32 i2 = 0; i2 < uiNumParams; ++i2)
        {
          inout_stream >> key;
          inout_stream >> value;

          ia.m_Parameters.Insert(key, value);
        }
      }
    }
  }

  if (uiVersion >= 8)
  {
    inout_stream >> m_iGroundType;
  }
}

void ezSurfaceResourceDescriptor::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 8;

  inout_stream << uiVersion;
  inout_stream << m_fPhysicsRestitution;
  inout_stream << m_fPhysicsFrictionStatic;
  inout_stream << m_fPhysicsFrictionDynamic;
  inout_stream << m_hBaseSurface;

  // version 4
  inout_stream << m_sOnCollideInteraction;

  // version 7
  inout_stream << m_sSlideInteractionPrefab;
  inout_stream << m_sRollInteractionPrefab;

  inout_stream << m_Interactions.GetCount();
  for (const auto& ia : m_Interactions)
  {
    inout_stream << ia.m_sInteractionType;
    inout_stream << ia.m_hPrefab;
    inout_stream << ia.m_Alignment;
    inout_stream << ia.m_Deviation;

    // version 4
    inout_stream << ia.m_fImpulseThreshold;

    // version 5
    inout_stream << ia.m_fImpulseScale;

    // version 6
    const ezUInt8 uiNumParams = static_cast<ezUInt8>(ia.m_Parameters.GetCount());
    inout_stream << uiNumParams;
    for (ezUInt32 i = 0; i < uiNumParams; ++i)
    {
      inout_stream << ia.m_Parameters.GetKey(i);
      inout_stream << ia.m_Parameters.GetValue(i);
    }
  }

  // version 8
  inout_stream << m_iGroundType;
}

void ezSurfaceResourceDescriptor::SetCollisionInteraction(const char* szName)
{
  m_sOnCollideInteraction.Assign(szName);
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

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    EZ_IGNORE_UNUSED(ref_context);
    EZ_IGNORE_UNUSED(pGraph);

    pNode->RenameProperty("Base Surface", "BaseSurface");
    pNode->RenameProperty("Static Friction", "StaticFriction");
    pNode->RenameProperty("Dynamic Friction", "DynamicFriction");
  }
};

ezSurfaceResourceDescriptorPatch_1_2 g_ezSurfaceResourceDescriptorPatch_1_2;


EZ_STATICLINK_FILE(Core, Core_Physics_Implementation_SurfaceResourceDescriptor);
