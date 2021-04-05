#include <RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/BoneWeightsAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/skeleton_utils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBoneWeightsAnimNode, 1, ezRTTIDefaultAllocator<ezBoneWeightsAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new ezDefaultValueAttribute(10.0)),
    EZ_ARRAY_ACCESSOR_PROPERTY("RootBones", RootBones_GetCount, RootBones_GetValue, RootBones_SetValue, RootBones_Insert, RootBones_Remove),

    EZ_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InverseWeights", m_InverseWeightsPin)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Weights"),
    new ezColorAttribute(ezColor::Teal),
    new ezTitleAttribute("Bone Weights"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBoneWeightsAnimNode::ezBoneWeightsAnimNode() = default;
ezBoneWeightsAnimNode::~ezBoneWeightsAnimNode() = default;

ezUInt32 ezBoneWeightsAnimNode::RootBones_GetCount() const
{
  return m_RootBones.GetCount();
}

const char* ezBoneWeightsAnimNode::RootBones_GetValue(ezUInt32 uiIndex) const
{
  return m_RootBones[uiIndex].GetString();
}

void ezBoneWeightsAnimNode::RootBones_SetValue(ezUInt32 uiIndex, const char* value)
{
  m_RootBones[uiIndex].Assign(value);
}

void ezBoneWeightsAnimNode::RootBones_Insert(ezUInt32 uiIndex, const char* value)
{
  ezHashedString tmp;
  tmp.Assign(value);
  m_RootBones.Insert(tmp, uiIndex);
}

void ezBoneWeightsAnimNode::RootBones_Remove(ezUInt32 uiIndex)
{
  m_RootBones.RemoveAtAndCopy(uiIndex);
}

ezResult ezBoneWeightsAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_RootBones));

  stream << m_fWeight;

  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InverseWeightsPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezBoneWeightsAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_RootBones));

  stream >> m_fWeight;

  EZ_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InverseWeightsPin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezBoneWeightsAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (m_RootBones.IsEmpty() || (!m_WeightsPin.IsConnected() && !m_InverseWeightsPin.IsConnected()))
    return;

  const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

  if (m_pBoneWeights == nullptr && m_pInverseBoneWeights == nullptr)
  {
    m_pBoneWeights = graph.AllocateBoneWeights(*pSkeleton);

    ezMemoryUtils::ZeroFill<ezUInt8>((ezUInt8*)m_pBoneWeights->m_ozzBoneWeights.data(), m_pBoneWeights->m_ozzBoneWeights.size() * sizeof(ozz::math::SimdFloat4));

    for (const auto& rootBone : m_RootBones)
    {
      int iRootBone = -1;
      for (int iBone = 0; iBone < pOzzSkeleton->num_joints(); ++iBone)
      {
        if (ezStringUtils::IsEqual(pOzzSkeleton->joint_names()[iBone], rootBone.GetData()))
        {
          iRootBone = iBone;
          break;
        }
      }

      const float fBoneWeight = 1.0f;

      auto setBoneWeight = [&](int currentBone, int) {
        const int iJointIdx0 = currentBone / 4;
        const int iJointIdx1 = currentBone % 4;

        ozz::math::SimdFloat4& soa_weight = m_pBoneWeights->m_ozzBoneWeights.at(iJointIdx0);
        soa_weight = ozz::math::SetI(soa_weight, ozz::math::simd_float4::Load1(fBoneWeight), iJointIdx1);
      };

      ozz::animation::IterateJointsDF(*pOzzSkeleton, setBoneWeight, iRootBone);
    }

    m_pBoneWeights->m_fOverallWeight = m_fWeight;

    if (m_InverseWeightsPin.IsConnected())
    {
      m_pInverseBoneWeights = graph.AllocateBoneWeights(*pSkeleton);
      m_pInverseBoneWeights->m_fOverallWeight = m_pBoneWeights->m_fOverallWeight;

      const ozz::math::SimdFloat4 oneBone = ozz::math::simd_float4::one();

      for (size_t b = 0; b < m_pInverseBoneWeights->m_ozzBoneWeights.size(); ++b)
      {
        m_pInverseBoneWeights->m_ozzBoneWeights[b] = ozz::math::MSub(oneBone, oneBone, m_pBoneWeights->m_ozzBoneWeights[b]);
      }
    }

    if (!m_WeightsPin.IsConnected())
    {
      graph.FreeBoneWeights(m_pBoneWeights);
    }
  }

  m_WeightsPin.SetWeights(graph, m_pBoneWeights);
  m_InverseWeightsPin.SetWeights(graph, m_pInverseBoneWeights);
}
