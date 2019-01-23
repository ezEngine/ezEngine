#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <SampleGamePlugin/Components/RedursiveGrowthComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(RecursiveGrowthComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Children", m_uiNumChildren)->AddAttributes(new ezDefaultValueAttribute(2), new ezClampValueAttribute(0, 200)),
    EZ_MEMBER_PROPERTY("RecursionDepth", m_uiRecursionDepth)->AddAttributes(new ezDefaultValueAttribute(2), new ezClampValueAttribute(0, 5)),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("SampleGame"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

RecursiveGrowthComponent::RecursiveGrowthComponent()
{
  m_uiNumChildren = 2;
  m_uiRecursionDepth = 2;
  m_uiChild = 0;
}

void RecursiveGrowthComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  // Version 1
  s << m_uiNumChildren;

  // Version 2
  s << m_uiRecursionDepth;

  // Version 3
}


void RecursiveGrowthComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_uiNumChildren;

  if (uiVersion >= 2) // version number given in EZ_BEGIN_COMPONENT_TYPE above
  {
    s >> m_uiRecursionDepth;
  }
}

void RecursiveGrowthComponent::OnSimulationStarted()
{
  RecursiveGrowthComponentManager* pManager = GetWorld()->GetComponentManager<RecursiveGrowthComponentManager>();

  EZ_LOG_BLOCK("RecursiveGrowthComponent::OnSimulationStarted");

  if (m_uiRecursionDepth > 0)
  {
    ezLog::Debug("Recursion Depth: {0}, Creating {1} children", m_uiRecursionDepth, m_uiNumChildren);

    for (ezUInt32 i = 0; i < m_uiNumChildren; ++i)
    {
      ezGameObjectDesc gd;
      gd.m_hParent = GetOwner()->GetHandle();
      gd.m_LocalPosition.Set(i * 2.0f, 0, 0);

      ezGameObject* pChild;
      GetWorld()->CreateObject(gd, pChild);

      RecursiveGrowthComponent* pChildComp = nullptr;
      ezComponentHandle hChildComp = pManager->CreateComponent(pChild, pChildComp);

      pChildComp->m_uiNumChildren = m_uiNumChildren;
      pChildComp->m_uiRecursionDepth = m_uiRecursionDepth - 1;
      pChildComp->m_uiChild = i;

      // TODO: Add more components (e.g. mesh) if desired
    }
  }
}

void RecursiveGrowthComponent::Update()
{
  // do stuff
}

void RecursiveGrowthComponent::Initialize()
{
  ezLog::Info("RecursiveGrowthComponent::Initialize: Child {0}, Recursions: {1}", m_uiChild, m_uiRecursionDepth);
}
