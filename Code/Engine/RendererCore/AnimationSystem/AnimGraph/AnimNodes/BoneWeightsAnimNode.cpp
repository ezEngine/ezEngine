#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/BoneWeightsAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/skeleton_utils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBoneWeightsAnimNode, 1, ezRTTIDefaultAllocator<ezBoneWeightsAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new ezDefaultValueAttribute(1.0), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ARRAY_ACCESSOR_PROPERTY("RootBones", RootBones_GetCount, RootBones_GetValue, RootBones_SetValue, RootBones_Insert, RootBones_Remove),

    EZ_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InverseWeights", m_InverseWeightsPin)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Weights"),
    new ezColorAttribute(ezColorScheme::GetColorForUI(ezColorScheme::Cyan)),
    new ezTitleAttribute("Bone Weights '{RootBones[0]}' '{RootBones[1]}' '{RootBones[2]}'"),
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
  if (!m_WeightsPin.IsConnected() && !m_InverseWeightsPin.IsConnected())
    return;

  if (m_RootBones.IsEmpty())
  {
    ezLog::Warning("No root-bones added to bone weight node in animation controller.");
    return;
  }

  if (m_pSharedBoneWeights == nullptr && m_pSharedInverseBoneWeights == nullptr)
  {
    const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

    ezStringBuilder name;
    name.Format("{}", pSkeleton->GetResourceIDHash());

    for (const auto& rootBone : m_RootBones)
    {
      name.AppendFormat("-{}", rootBone);
    }

    m_pSharedBoneWeights = graph.CreateBoneWeights(name, *pSkeleton, [this, pOzzSkeleton](ezAnimGraphSharedBoneWeights& bw) {
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

          ozz::math::SimdFloat4& soa_weight = bw.m_Weights[iJointIdx0];
          soa_weight = ozz::math::SetI(soa_weight, ozz::math::simd_float4::Load1(fBoneWeight), iJointIdx1);
        };

        ozz::animation::IterateJointsDF(*pOzzSkeleton, setBoneWeight, iRootBone);
      }
    });

    if (m_InverseWeightsPin.IsConnected())
    {
      name.Append("-inv");

      m_pSharedInverseBoneWeights = graph.CreateBoneWeights(name, *pSkeleton, [this](ezAnimGraphSharedBoneWeights& bw) {
        const ozz::math::SimdFloat4 oneBone = ozz::math::simd_float4::one();

        for (ezUInt32 b = 0; b < bw.m_Weights.GetCount(); ++b)
        {
          bw.m_Weights[b] = ozz::math::MSub(oneBone, oneBone, m_pSharedBoneWeights->m_Weights[b]);
        }
      });
    }

    if (!m_WeightsPin.IsConnected())
    {
      m_pSharedBoneWeights.Clear();
    }
  }

  if (m_WeightsPin.IsConnected())
  {
    ezAnimGraphPinDataBoneWeights* pPinData = graph.AddPinDataBoneWeights();
    pPinData->m_fOverallWeight = m_fWeight;
    pPinData->m_pSharedBoneWeights = m_pSharedBoneWeights.Borrow();

    m_WeightsPin.SetWeights(graph, pPinData);
  }

  if (m_InverseWeightsPin.IsConnected())
  {
    ezAnimGraphPinDataBoneWeights* pPinData = graph.AddPinDataBoneWeights();
    pPinData->m_fOverallWeight = m_fWeight;
    pPinData->m_pSharedBoneWeights = m_pSharedInverseBoneWeights.Borrow();

    m_InverseWeightsPin.SetWeights(graph, pPinData);
  }
}
