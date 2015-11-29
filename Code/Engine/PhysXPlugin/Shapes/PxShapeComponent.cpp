#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/PhysXSceneModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxShapeComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPxShapeComponent::ezPxShapeComponent()
{
}


void ezPxShapeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  /// \todo Serialize resource handles more efficiently
  s << GetSurfaceFile();
}


void ezPxShapeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  ezStringBuilder sTemp;
  s >> sTemp;
  SetSurfaceFile(sTemp);
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

  ezResourceLock<ezSurfaceResource> pResource(m_hSurface);
  return pResource->GetResourceID();
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
