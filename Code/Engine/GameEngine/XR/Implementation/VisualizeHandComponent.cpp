#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/VisualizeHandComponent.h>
#include <GameEngine/XR/XRHandTrackingInterface.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/DebugRendererContext.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezVisualizeHandComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("XR"),
    new ezInDevelopmentAttribute(ezInDevelopmentAttribute::Phase::Beta),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezVisualizeHandComponent::ezVisualizeHandComponent() = default;
ezVisualizeHandComponent::~ezVisualizeHandComponent() = default;

void ezVisualizeHandComponent::Update()
{
  ezXRHandTrackingInterface* pXRHand = ezSingletonRegistry::GetSingletonInstance<ezXRHandTrackingInterface>();

  if (!pXRHand)
    return;

  ezHybridArray<ezXRHandBone, 6> bones;
  for (ezXRHand::Enum hand : {ezXRHand::Left, ezXRHand::Right})
  {
    for (ezUInt32 uiPart = 0; uiPart < ezXRHandPart::COUNT; ++uiPart)
    {
      ezXRHandPart::Enum part = static_cast<ezXRHandPart::Enum>(uiPart);
      if (pXRHand->TryGetBoneTransforms(hand, part, ezXRTransformSpace::Global, bones) == ezXRHandTrackingInterface::HandPartTrackingState::Tracked)
      {
        ezHybridArray<ezDebugRendererLine, 6> m_Lines;
        for (ezUInt32 uiBone = 0; uiBone < bones.GetCount(); uiBone++)
        {
          const ezXRHandBone& bone = bones[uiBone];
          ezBoundingSphere sphere = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), bone.m_fRadius);
          ezDebugRenderer::DrawLineSphere(GetWorld(), sphere, ezColor::Aquamarine, bone.m_Transform);

          if (uiBone + 1 < bones.GetCount())
          {
            const ezXRHandBone& nextBone = bones[uiBone + 1];
            m_Lines.PushBack(ezDebugRendererLine(bone.m_Transform.m_vPosition, nextBone.m_Transform.m_vPosition));
          }
        }
        ezDebugRenderer::DrawLines(GetWorld(), m_Lines, ezColor::IndianRed);
      }
    }
  }
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_VisualizeHandComponent);
