#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/PhysXWorldModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezPxShapeComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
    EZ_MEMBER_PROPERTY("Collision Layer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("Physics/Shapes"),
  EZ_END_ATTRIBUTES
EZ_END_ABSTRACT_COMPONENT_TYPE();

ezPxShapeComponent::ezPxShapeComponent()
{
  m_uiCollisionLayer = 0;
}


void ezPxShapeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hSurface;
  s << m_uiCollisionLayer;
}


void ezPxShapeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();

  s >> m_hSurface;
  s >> m_uiCollisionLayer;
}

void ezPxShapeComponent::SetSurfaceFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    ezResourceManager::PreloadResource(m_hSurface, ezTime::Seconds(5.0));

}

const char* ezPxShapeComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

PxMaterial* ezPxShapeComponent::GetPxMaterial()
{
  if (m_hSurface.IsValid())
  {
    ezResourceLock<ezSurfaceResource> pSurface(m_hSurface);

    if (pSurface->m_pPhysicsMaterial != nullptr)
    {
      return static_cast<PxMaterial*>(pSurface->m_pPhysicsMaterial);
    }
  }

  return ezPhysX::GetSingleton()->GetDefaultMaterial();
}
