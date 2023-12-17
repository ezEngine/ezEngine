#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>

ezAnimGraphInstance::ezAnimGraphInstance() = default;

ezAnimGraphInstance::~ezAnimGraphInstance()
{
  if (m_pAnimGraph)
  {
    m_pAnimGraph->GetInstanceDataAlloator().DestructAndDeallocate(m_InstanceData);
  }
}

void ezAnimGraphInstance::Configure(const ezAnimGraph& animGraph)
{
  m_pAnimGraph = &animGraph;

  m_InstanceData = m_pAnimGraph->GetInstanceDataAlloator().AllocateAndConstruct();

  // EXTEND THIS if a new type is introduced
  m_pTriggerInputPinStates = (ezInt8*)ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::Trigger]);
  m_pNumberInputPinStates = (double*)ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::Number]);
  m_pBoolInputPinStates = (bool*)ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::Bool]);
  m_pBoneWeightInputPinStates = (ezUInt16*)ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::BoneWeights]);
  m_pModelPoseInputPinStates = (ezUInt16*)ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[ezAnimGraphPin::Type::ModelPose]);

  m_LocalPoseInputPinStates.SetCount(animGraph.m_uiInputPinCounts[ezAnimGraphPin::Type::LocalPose]);
}

void ezAnimGraphInstance::Update(ezAnimController& ref_controller, ezTime diff, ezGameObject* pTarget, const ezSkeletonResource* pSekeltonResource)
{
  // reset all pin states
  {
    // EXTEND THIS if a new type is introduced

    ezMemoryUtils::ZeroFill(m_pTriggerInputPinStates, m_pAnimGraph->m_uiInputPinCounts[ezAnimGraphPin::Type::Trigger]);
    ezMemoryUtils::ZeroFill(m_pNumberInputPinStates, m_pAnimGraph->m_uiInputPinCounts[ezAnimGraphPin::Type::Number]);
    ezMemoryUtils::ZeroFill(m_pBoolInputPinStates, m_pAnimGraph->m_uiInputPinCounts[ezAnimGraphPin::Type::Bool]);
    ezMemoryUtils::ZeroFill(m_pBoneWeightInputPinStates, m_pAnimGraph->m_uiInputPinCounts[ezAnimGraphPin::Type::BoneWeights]);
    ezMemoryUtils::PatternFill(m_pModelPoseInputPinStates, 0xFF, m_pAnimGraph->m_uiInputPinCounts[ezAnimGraphPin::Type::ModelPose]);

    for (auto& pins : m_LocalPoseInputPinStates)
    {
      pins.Clear();
    }
  }

  for (const auto& pNode : m_pAnimGraph->GetNodes())
  {
    pNode->Step(ref_controller, *this, diff, pSekeltonResource, pTarget);
  }
}


