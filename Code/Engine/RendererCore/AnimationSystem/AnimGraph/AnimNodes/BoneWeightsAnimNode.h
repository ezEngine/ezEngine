#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class ezSkeletonResource;
class ezStreamWriter;
class ezStreamReader;

class EZ_RENDERERCORE_DLL ezBoneWeightsAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBoneWeightsAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezBoneWeightsAnimNode

public:
  ezBoneWeightsAnimNode();
  ~ezBoneWeightsAnimNode();

  float m_fWeight = 1.0f;                                       // [ property ]

  ezUInt32 RootBones_GetCount() const;                          // [ property ]
  const char* RootBones_GetValue(ezUInt32 uiIndex) const;       // [ property ]
  void RootBones_SetValue(ezUInt32 uiIndex, const char* value); // [ property ]
  void RootBones_Insert(ezUInt32 uiIndex, const char* value);   // [ property ]
  void RootBones_Remove(ezUInt32 uiIndex);                      // [ property ]

private:
  ezAnimGraphBoneWeightsOutputPin m_WeightsPin;                 // [ property ]
  ezAnimGraphBoneWeightsOutputPin m_InverseWeightsPin;          // [ property ]

  ezHybridArray<ezHashedString, 2> m_RootBones;

  struct InstanceData
  {
    ezSharedPtr<ezAnimGraphSharedBoneWeights> m_pSharedBoneWeights;
    ezSharedPtr<ezAnimGraphSharedBoneWeights> m_pSharedInverseBoneWeights;
  };
};
