#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <ozz/animation/runtime/skeleton.h>

ezMutex ezAnimController::s_SharedDataMutex;
ezHashTable<ezString, ezSharedPtr<ezAnimGraphSharedBoneWeights>> ezAnimController::s_SharedBoneWeights;

ezAnimController::ezAnimController() = default;
ezAnimController::~ezAnimController() = default;

void ezAnimController::Initialize(const ezSkeletonResourceHandle& hSkeleton, ezAnimPoseGenerator& ref_poseGenerator, const ezSharedPtr<ezBlackboard>& pBlackboard /*= nullptr*/)
{
  m_Instances.Clear();
  m_PinDataBoneWeights.Clear();
  m_PinDataLocalTransforms.Clear();
  m_PinDataModelTransforms.Clear();
  m_AnimationClipMapping.Clear();
  m_CurrentLocalTransformOutputs.Clear();
  m_pBlackboard.Clear();
  m_BlendMask.Clear();
  m_pPoseGenerator = nullptr;
  m_pCurrentModelTransforms = nullptr;
  m_hSkeleton = {};
  m_vRootMotion.SetZero();
  m_RootRotationX = {};
  m_RootRotationY = {};
  m_RootRotationZ = {};

  m_hSkeleton = hSkeleton;
  m_pPoseGenerator = &ref_poseGenerator;
  m_pBlackboard = pBlackboard;
}

void ezAnimController::GetRootMotion(ezVec3& ref_vTranslation, ezAngle& ref_rotationX, ezAngle& ref_rotationY, ezAngle& ref_rotationZ) const
{
  ref_vTranslation = m_vRootMotion;
  ref_rotationX = m_RootRotationX;
  ref_rotationY = m_RootRotationY;
  ref_rotationZ = m_RootRotationZ;
}

bool ezAnimController::Update(ezTime diff, ezGameObject* pTarget, bool bEnableIK)
{
  if (!m_hSkeleton.IsValid())
    return false;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return false;

  m_pCurrentModelTransforms = nullptr;

  m_CurrentLocalTransformOutputs.Clear();

  m_vRootMotion = ezVec3::MakeZero();
  m_RootRotationX = {};
  m_RootRotationY = {};
  m_RootRotationZ = {};

  m_pPoseGenerator->Reset(pSkeleton.GetPointer(), pTarget);

  m_PinDataBoneWeights.Clear();
  m_PinDataLocalTransforms.Clear();
  m_PinDataModelTransforms.Clear();

  for (auto& inst : m_Instances)
  {
    inst.m_pInstance->Update(*this, diff, pTarget, pSkeleton.GetPointer());
  }

  GenerateLocalResultProcessors(pSkeleton.GetPointer());

  GetPoseGenerator().UpdatePose(bEnableIK);

  if (GetPoseGenerator().ShouldSendPoseResultMsg())
  {
    if (auto newPose = GetPoseGenerator().GetCurrentPose(); !newPose.IsEmpty())
    {
      ezMsgAnimationPoseUpdated msg;
      msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
      msg.m_ModelTransforms = newPose;

      // TODO: root transform has to be applied first, only then can the world-space IK be done, and then the pose can be finalized
      msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;

      // recursive, so that objects below the mesh can also listen in on these changes
      // for example bone attachments
      pTarget->SendMessageRecursive(msg);

      return msg.m_bContinueAnimating;
    }
  }

  return true;
}

void ezAnimController::SetOutputModelTransform(ezAnimGraphPinDataModelTransforms* pModelTransform)
{
  m_pCurrentModelTransforms = pModelTransform;
}

void ezAnimController::SetRootMotion(const ezVec3& vTranslation, ezAngle rotationX, ezAngle rotationY, ezAngle rotationZ)
{
  m_vRootMotion = vTranslation;
  m_RootRotationX = rotationX;
  m_RootRotationY = rotationY;
  m_RootRotationZ = rotationZ;
}

void ezAnimController::AddOutputLocalTransforms(ezAnimGraphPinDataLocalTransforms* pLocalTransforms)
{
  m_CurrentLocalTransformOutputs.PushBack(pLocalTransforms->m_uiOwnIndex);
}

ezSharedPtr<ezAnimGraphSharedBoneWeights> ezAnimController::CreateBoneWeights(const char* szUniqueName, const ezSkeletonResource& skeleton, ezDelegate<void(ezAnimGraphSharedBoneWeights&)> fill)
{
  EZ_LOCK(s_SharedDataMutex);

  ezSharedPtr<ezAnimGraphSharedBoneWeights>& bw = s_SharedBoneWeights[szUniqueName];

  if (bw == nullptr)
  {
    bw = EZ_DEFAULT_NEW(ezAnimGraphSharedBoneWeights);
    bw->m_Weights.SetCountUninitialized(skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());
    ezMemoryUtils::ZeroFill<ozz::math::SimdFloat4>(bw->m_Weights.GetData(), bw->m_Weights.GetCount());
  }

  fill(*bw);

  return bw;
}

void ezAnimController::GenerateLocalResultProcessors(const ezSkeletonResource* pSkeleton)
{
  if (m_CurrentLocalTransformOutputs.IsEmpty())
    return;

  ezAnimGraphPinDataLocalTransforms* pOut = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[0]];

  // combine multiple outputs
  if (m_CurrentLocalTransformOutputs.GetCount() > 1 || pOut->m_pWeights != nullptr)
  {
    const ezUInt32 m_uiMaxPoses = 6; // TODO

    pOut = AddPinDataLocalTransforms();
    pOut->m_vRootMotion.SetZero();

    float fSummedRootMotionWeight = 0.0f;

    // TODO: skip blending, if only a single animation is played
    // unless the weight is below 1.0 and the bind pose should be faded in

    auto& cmd = GetPoseGenerator().AllocCommandCombinePoses();

    struct PinWeight
    {
      ezUInt32 m_uiPinIdx;
      float m_fPinWeight = 0.0f;
    };

    ezHybridArray<PinWeight, 16> pw;
    pw.SetCount(m_CurrentLocalTransformOutputs.GetCount());

    for (ezUInt32 i = 0; i < m_CurrentLocalTransformOutputs.GetCount(); ++i)
    {
      pw[i].m_uiPinIdx = i;

      const ezAnimGraphPinDataLocalTransforms* pTransforms = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[i]];

      if (pTransforms != nullptr)
      {
        pw[i].m_fPinWeight = pTransforms->m_fOverallWeight;

        if (pTransforms->m_pWeights)
        {
          pw[i].m_fPinWeight *= pTransforms->m_pWeights->m_fOverallWeight;
        }
      }
    }

    if (pw.GetCount() > m_uiMaxPoses)
    {
      pw.Sort([](const PinWeight& lhs, const PinWeight& rhs)
        { return lhs.m_fPinWeight > rhs.m_fPinWeight; });
      pw.SetCount(m_uiMaxPoses);
    }

    ezArrayPtr<const ozz::math::SimdFloat4> invWeights;

    for (const auto& in : pw)
    {
      const ezAnimGraphPinDataLocalTransforms* pTransforms = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[in.m_uiPinIdx]];

      if (in.m_fPinWeight > 0 && pTransforms->m_pWeights)
      {
        // only initialize and use the inverse mask, when it is actually needed
        if (invWeights.IsEmpty())
        {
          m_BlendMask.SetCountUninitialized(pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());

          for (auto& sj : m_BlendMask)
          {
            sj = ozz::math::simd_float4::one();
          }

          invWeights = m_BlendMask;
        }

        const ozz::math::SimdFloat4 factor = ozz::math::simd_float4::Load1(in.m_fPinWeight);

        const ezArrayPtr<const ozz::math::SimdFloat4> weights = pTransforms->m_pWeights->m_pSharedBoneWeights->m_Weights;

        for (ezUInt32 i = 0; i < m_BlendMask.GetCount(); ++i)
        {
          const auto& weight = weights[i];
          auto& mask = m_BlendMask[i];

          const auto oneMinusWeight = ozz::math::NMAdd(factor, weight, ozz::math::simd_float4::one());

          mask = ozz::math::Min(mask, oneMinusWeight);
        }
      }
    }

    for (const auto& in : pw)
    {
      if (in.m_fPinWeight > 0)
      {
        const ezAnimGraphPinDataLocalTransforms* pTransforms = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[in.m_uiPinIdx]];

        if (pTransforms->m_pWeights)
        {
          const ezArrayPtr<const ozz::math::SimdFloat4> weights = pTransforms->m_pWeights->m_pSharedBoneWeights->m_Weights;

          cmd.m_InputBoneWeights.PushBack(weights);
        }
        else
        {
          cmd.m_InputBoneWeights.PushBack(invWeights);
        }

        if (pTransforms->m_bUseRootMotion)
        {
          fSummedRootMotionWeight += in.m_fPinWeight;
          pOut->m_vRootMotion += pTransforms->m_vRootMotion * in.m_fPinWeight;

          // TODO: combining quaternions is mathematically tricky
          // could maybe use multiple slerps to concatenate weighted quaternions \_(ツ)_/

          pOut->m_bUseRootMotion = true;
        }

        cmd.m_Inputs.PushBack(pTransforms->m_CommandID);
        cmd.m_InputWeights.PushBack(in.m_fPinWeight);
      }
    }

    if (fSummedRootMotionWeight > 1.0f) // normalize down, but not up
    {
      pOut->m_vRootMotion /= fSummedRootMotionWeight;
    }

    pOut->m_CommandID = cmd.GetCommandID();
  }

  ezAnimGraphPinDataModelTransforms* pModelTransform = AddPinDataModelTransforms();

  // local space to model space
  {
    if (pOut->m_bUseRootMotion)
    {
      pModelTransform->m_bUseRootMotion = true;
      pModelTransform->m_vRootMotion = pOut->m_vRootMotion;
    }

    auto& cmd = GetPoseGenerator().AllocCommandLocalToModelPose();
    cmd.m_Inputs.PushBack(pOut->m_CommandID);

    pModelTransform->m_CommandID = cmd.GetCommandID();
  }

  // model space to output
  {
    ezVec3 rootMotion = ezVec3::MakeZero();
    ezAngle rootRotationX;
    ezAngle rootRotationY;
    ezAngle rootRotationZ;
    GetRootMotion(rootMotion, rootRotationX, rootRotationY, rootRotationZ);

    GetPoseGenerator().SetFinalCommand(pModelTransform->m_CommandID);

    if (pModelTransform->m_bUseRootMotion)
    {
      rootMotion += pModelTransform->m_vRootMotion;
      rootRotationX += pModelTransform->m_RootRotationX;
      rootRotationY += pModelTransform->m_RootRotationY;
      rootRotationZ += pModelTransform->m_RootRotationZ;
    }

    SetOutputModelTransform(pModelTransform);

    SetRootMotion(rootMotion, rootRotationX, rootRotationY, rootRotationZ);
  }
}

ezAnimGraphPinDataBoneWeights* ezAnimController::AddPinDataBoneWeights()
{
  ezAnimGraphPinDataBoneWeights* pData = &m_PinDataBoneWeights.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<ezUInt16>(m_PinDataBoneWeights.GetCount()) - 1;
  return pData;
}

ezAnimGraphPinDataLocalTransforms* ezAnimController::AddPinDataLocalTransforms()
{
  ezAnimGraphPinDataLocalTransforms* pData = &m_PinDataLocalTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<ezUInt16>(m_PinDataLocalTransforms.GetCount()) - 1;
  return pData;
}

ezAnimGraphPinDataModelTransforms* ezAnimController::AddPinDataModelTransforms()
{
  ezAnimGraphPinDataModelTransforms* pData = &m_PinDataModelTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<ezUInt16>(m_PinDataModelTransforms.GetCount()) - 1;
  return pData;
}

void ezAnimController::AddAnimGraph(const ezAnimGraphResourceHandle& hGraph)
{
  if (!hGraph.IsValid())
    return;

  for (auto& inst : m_Instances)
  {
    if (inst.m_hAnimGraph == hGraph)
      return;
  }

  ezResourceLock<ezAnimGraphResource> pAnimGraph(hGraph, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimGraph.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  auto& inst = m_Instances.ExpandAndGetRef();
  inst.m_hAnimGraph = hGraph;
  inst.m_pInstance = EZ_DEFAULT_NEW(ezAnimGraphInstance);
  inst.m_pInstance->Configure(pAnimGraph->GetAnimationGraph());

  for (auto& clip : pAnimGraph->GetAnimationClipMapping())
  {
    bool bExisted = false;
    auto& info = m_AnimationClipMapping.FindOrAdd(clip.m_sClipName, &bExisted);
    if (!bExisted)
    {
      info.m_hClip = clip.m_hClip;
    }
  }

  for (auto& ig : pAnimGraph->GetIncludeGraphs())
  {
    AddAnimGraph(ezResourceManager::LoadResource<ezAnimGraphResource>(ig));
  }
}

const ezAnimController::AnimClipInfo& ezAnimController::GetAnimationClipInfo(ezTempHashedString sClipName) const
{
  auto it = m_AnimationClipMapping.Find(sClipName);
  if (!it.IsValid())
    return m_InvalidClipInfo;

  return it.Value();
}

void ezAnimController::SetAnimationClipInfo(const ezHashedString& sClipName, const AnimClipInfo& info)
{
  m_AnimationClipMapping[sClipName] = info;
}
