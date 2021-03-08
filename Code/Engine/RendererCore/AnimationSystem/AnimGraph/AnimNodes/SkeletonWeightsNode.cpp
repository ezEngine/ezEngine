#include <RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/SkeletonWeightsNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/skeleton_utils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonWeightsAnimNode, 1, ezRTTIDefaultAllocator<ezSkeletonWeightsAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("PartialBlendingRootBone", GetPartialBlendingRootBone, SetPartialBlendingRootBone),

      EZ_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new ezDefaultValueAttribute(10.0)),
      EZ_MEMBER_PROPERTY("Weights", m_Weights)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSkeletonWeightsAnimNode::ezSkeletonWeightsAnimNode() = default;

ezSkeletonWeightsAnimNode::~ezSkeletonWeightsAnimNode()
{
  if (m_pPartialBlendingMask)
  {
    // TODO: destroy ? (would be cleaned up by graph anyway)
  }
}


ezResult ezSkeletonWeightsAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sPartialBlendingRootBone;
  stream << m_fWeight;

  EZ_SUCCEED_OR_RETURN(m_Weights.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSkeletonWeightsAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sPartialBlendingRootBone;
  stream >> m_fWeight;

  EZ_SUCCEED_OR_RETURN(m_Weights.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSkeletonWeightsAnimNode::Step(ezAnimGraph* pOwner, ezTime tDiff, const ezSkeletonResource* pSkeleton)
{
  if (!m_Weights.IsConnected())
    return;

  const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  if (m_sPartialBlendingRootBone.IsEmpty())
  {
    pOwner->FreeBlendWeights(m_pPartialBlendingMask);
  }
  else
  {
    if (m_pPartialBlendingMask == nullptr)
    {
      m_pPartialBlendingMask = pOwner->AllocateBlendWeights(*pSkeleton);

      ezMemoryUtils::ZeroFill<ezUInt8>((ezUInt8*)m_pPartialBlendingMask->m_ozzBlendWeights.data(), m_pPartialBlendingMask->m_ozzBlendWeights.size() * sizeof(ozz::math::SimdFloat4));

      int iRootBone = -1;
      for (int iBone = 0; iBone < pOzzSkeleton->num_joints(); ++iBone)
      {
        if (ezStringUtils::IsEqual(pOzzSkeleton->joint_names()[iBone], m_sPartialBlendingRootBone.GetData()))
        {
          iRootBone = iBone;
          break;
        }
      }

      const float fBoneWeight = 1.0f;

      auto setBoneWeight = [&](int currentBone, int) {
        const int iJointIdx0 = currentBone / 4;
        const int iJointIdx1 = currentBone % 4;

        ozz::math::SimdFloat4& soa_weight = m_pPartialBlendingMask->m_ozzBlendWeights.at(iJointIdx0);
        soa_weight = ozz::math::SetI(soa_weight, ozz::math::simd_float4::Load1(fBoneWeight), iJointIdx1);
      };

      ozz::animation::IterateJointsDF(*pOzzSkeleton, setBoneWeight, iRootBone);
    }
  }

  if (m_pPartialBlendingMask)
  {
    // TODO: use proper inverse mask instead of large weight trick
    m_pPartialBlendingMask->m_fOverallWeight = m_fWeight;
  }

  m_Weights.SetWeights(*pOwner, m_pPartialBlendingMask);
}

void ezSkeletonWeightsAnimNode::SetPartialBlendingRootBone(const char* szBone)
{
  m_sPartialBlendingRootBone.Assign(szBone);
}

const char* ezSkeletonWeightsAnimNode::GetPartialBlendingRootBone() const
{
  return m_sPartialBlendingRootBone.GetData();
}
