#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezPxShapeComponent, 2)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("ReportContact", m_bReportContact)
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Shapes"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_ABSTRACT_COMPONENT_TYPE

ezPxShapeComponent::ezPxShapeComponent()
  : m_uiCollisionLayer(0)
  , m_bReportContact(false)
  , m_uiShapeId(ezInvalidIndex)
  , m_UserData(this)
{
}

ezPxShapeComponent::~ezPxShapeComponent()
{

}

void ezPxShapeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hSurface;
  s << m_uiCollisionLayer;
  s << m_bReportContact;
}


void ezPxShapeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hSurface;
  s >> m_uiCollisionLayer;

  if (uiVersion >= 2)
  {
    s >> m_bReportContact;
  }
}


void ezPxShapeComponent::Initialize()
{
  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezPxShapeComponent::Deinitialize()
{
  if (m_uiShapeId != ezInvalidIndex)
  {
    if (ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>())
    {
      pModule->DeleteShapeId(m_uiShapeId);
      m_uiShapeId = ezInvalidIndex;
    }
  }
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

void ezPxShapeComponent::AddToActor(PxRigidActor* pActor, const ezTransform& parentTransform)
{
  PxTransform shapeTransform(PxIdentity);
  PxShape* pShape = CreateShape(pActor, shapeTransform);
  EZ_ASSERT_DEBUG(pShape != nullptr, "PhysX shape creation failed");

  const ezTransform ownerTransform = GetOwner()->GetGlobalTransform();

  ezTransform localTransform;
  localTransform.SetLocalTransform(parentTransform, ownerTransform);

  PxTransform t = ezPxConversionUtils::ToTransform(localTransform);
  pShape->setLocalPose(shapeTransform * t);

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  m_uiShapeId = pModule->CreateShapeId();

  PxFilterData filter = CreateFilterData();
  pShape->setSimulationFilterData(filter);
  pShape->setQueryFilterData(filter);

  pShape->userData = &m_UserData;
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

PxFilterData ezPxShapeComponent::CreateFilterData()
{
  return ezPhysX::CreateFilterData(m_uiCollisionLayer, m_uiShapeId, m_bReportContact);
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Shapes_Implementation_PxShapeComponent);

