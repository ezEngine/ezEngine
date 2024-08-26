#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>

namespace
{
  static ezVariantArray GetDefaultTags()
  {
    ezVariantArray value(ezStaticsAllocatorWrapper::GetAllocator());
    value.PushBack("SkyLight");
    return value;
  }
} // namespace

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSkyLightComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("ReflectionProbeMode", ezReflectionProbeMode, GetReflectionProbeMode, SetReflectionProbeMode)->AddAttributes(new ezDefaultValueAttribute(ezReflectionProbeMode::Dynamic), new ezGroupAttribute("Capture Description")),
    EZ_ACCESSOR_PROPERTY("CubeMap", GetCubeMapFile, SetCubeMapFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    EZ_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("Saturation", GetSaturation, SetSaturation)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_SET_ACCESSOR_PROPERTY("IncludeTags", GetIncludeTags, InsertIncludeTag, RemoveIncludeTag)->AddAttributes(new ezTagSetWidgetAttribute("Default"), new ezDefaultValueAttribute(GetDefaultTags())),
    EZ_SET_ACCESSOR_PROPERTY("ExcludeTags", GetExcludeTags, InsertExcludeTag, RemoveExcludeTag)->AddAttributes(new ezTagSetWidgetAttribute("Default")),
    EZ_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, {}), new ezMinValueTextAttribute("Auto")),
    EZ_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.01f, 10000.0f)),
    EZ_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    EZ_ACCESSOR_PROPERTY("ShowMipMaps", GetShowMipMaps, SetShowMipMaps),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgTransformChanged, OnTransformChanged),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Lighting"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSkyLightComponent::ezSkyLightComponent()
{
  m_Desc.m_uniqueID = ezUuid::MakeUuid();
}

ezSkyLightComponent::~ezSkyLightComponent() = default;

void ezSkyLightComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = ezReflectionPool::RegisterSkyLight(GetWorld(), m_Desc, this);

  GetOwner()->UpdateLocalBounds();
}

void ezSkyLightComponent::OnDeactivated()
{
  ezReflectionPool::DeregisterSkyLight(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void ezSkyLightComponent::SetReflectionProbeMode(ezEnum<ezReflectionProbeMode> mode)
{
  m_Desc.m_Mode = mode;
  m_bStatesDirty = true;
}

ezEnum<ezReflectionProbeMode> ezSkyLightComponent::GetReflectionProbeMode() const
{
  return m_Desc.m_Mode;
}

void ezSkyLightComponent::SetIntensity(float fIntensity)
{
  m_Desc.m_fIntensity = fIntensity;
  m_bStatesDirty = true;
}

float ezSkyLightComponent::GetIntensity() const
{
  return m_Desc.m_fIntensity;
}

void ezSkyLightComponent::SetSaturation(float fSaturation)
{
  m_Desc.m_fSaturation = fSaturation;
  m_bStatesDirty = true;
}

float ezSkyLightComponent::GetSaturation() const
{
  return m_Desc.m_fSaturation;
}

const ezTagSet& ezSkyLightComponent::GetIncludeTags() const
{
  return m_Desc.m_IncludeTags;
}

void ezSkyLightComponent::InsertIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void ezSkyLightComponent::RemoveIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

const ezTagSet& ezSkyLightComponent::GetExcludeTags() const
{
  return m_Desc.m_ExcludeTags;
}

void ezSkyLightComponent::InsertExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void ezSkyLightComponent::RemoveExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

void ezSkyLightComponent::SetShowDebugInfo(bool bShowDebugInfo)
{
  m_Desc.m_bShowDebugInfo = bShowDebugInfo;
  m_bStatesDirty = true;
}

bool ezSkyLightComponent::GetShowDebugInfo() const
{
  return m_Desc.m_bShowDebugInfo;
}

void ezSkyLightComponent::SetShowMipMaps(bool bShowMipMaps)
{
  m_Desc.m_bShowMipMaps = bShowMipMaps;
  m_bStatesDirty = true;
}

bool ezSkyLightComponent::GetShowMipMaps() const
{
  return m_Desc.m_bShowMipMaps;
}

void ezSkyLightComponent::SetCubeMapFile(ezStringView sFile)
{
  ezTextureCubeResourceHandle hCubeMap;

  if (!sFile.IsEmpty())
  {
    hCubeMap = ezResourceManager::LoadResource<ezTextureCubeResource>(sFile);
  }

  m_hCubeMap = hCubeMap;
  m_bStatesDirty = true;
}

ezStringView ezSkyLightComponent::GetCubeMapFile() const
{
  return m_hCubeMap.GetResourceID();
}

void ezSkyLightComponent::SetNearPlane(float fNearPlane)
{
  m_Desc.m_fNearPlane = fNearPlane;
  m_bStatesDirty = true;
}

void ezSkyLightComponent::SetFarPlane(float fFarPlane)
{
  m_Desc.m_fFarPlane = fFarPlane;
  m_bStatesDirty = true;
}

void ezSkyLightComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic);
}

void ezSkyLightComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't trigger reflection rendering in shadow or other reflection views.
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Reflection)
    return;

  if (m_bStatesDirty)
  {
    m_bStatesDirty = false;
    ezReflectionPool::UpdateSkyLight(GetWorld(), m_Id, m_Desc, this);
  }

  ezReflectionPool::ExtractReflectionProbe(this, msg, nullptr, GetWorld(), m_Id, ezMath::MaxValue<float>());
}

void ezSkyLightComponent::OnTransformChanged(ezMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void ezSkyLightComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Save(s);
  m_Desc.m_ExcludeTags.Save(s);
  s << m_Desc.m_Mode;
  s << m_Desc.m_bShowDebugInfo;
  s << m_Desc.m_fIntensity;
  s << m_Desc.m_fSaturation;
  s << m_hCubeMap;
  s << m_Desc.m_fNearPlane;
  s << m_Desc.m_fFarPlane;
}

void ezSkyLightComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Load(s, ezTagRegistry::GetGlobalRegistry());
  m_Desc.m_ExcludeTags.Load(s, ezTagRegistry::GetGlobalRegistry());
  s >> m_Desc.m_Mode;
  s >> m_Desc.m_bShowDebugInfo;
  s >> m_Desc.m_fIntensity;
  s >> m_Desc.m_fSaturation;
  if (uiVersion >= 2)
  {
    s >> m_hCubeMap;
  }
  if (uiVersion >= 3)
  {
    s >> m_Desc.m_fNearPlane;
    s >> m_Desc.m_fFarPlane;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezSkyLightComponentPatch_2_3 : public ezGraphPatch
{
public:
  ezSkyLightComponentPatch_2_3()
    : ezGraphPatch("ezSkyLightComponent", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    // Inline ReflectionData sub-object into the sky light itself.
    if (const ezAbstractObjectNode::Property* pProp0 = pNode->FindProperty("ReflectionData"))
    {
      if (pProp0->m_Value.IsA<ezUuid>())
      {
        if (ezAbstractObjectNode* pSubNode = pGraph->GetNode(pProp0->m_Value.Get<ezUuid>()))
        {
          for (auto pProp : pSubNode->GetProperties())
          {
            pNode->AddProperty(pProp.m_sPropertyName, pProp.m_Value);
          }
        }
      }
    }
  }
};

ezSkyLightComponentPatch_2_3 g_ezSkyLightComponentPatch_2_3;

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SkyLightComponent);
