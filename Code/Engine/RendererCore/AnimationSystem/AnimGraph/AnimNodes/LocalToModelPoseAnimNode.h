#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

// class EZ_RENDERERCORE_DLL ezLocalToModelPoseAnimNode : public ezAnimGraphNode
//{
//   EZ_ADD_DYNAMIC_REFLECTION(ezLocalToModelPoseAnimNode, ezAnimGraphNode);
//
//   //////////////////////////////////////////////////////////////////////////
//   // ezAnimGraphNode
//
// protected:
//   virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
//   virtual ezResult DeserializeNode(ezStreamReader& stream) override;
//
//   virtual void Step(ezAnimGraphExecutor& executor, ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
//
//   //////////////////////////////////////////////////////////////////////////
//   // ezLocalToModelPoseAnimNode
//
// public:
//   ezLocalToModelPoseAnimNode();
//   ~ezLocalToModelPoseAnimNode();
//
// private:
//   ezAnimGraphLocalPoseInputPin m_LocalPosePin;  // [ property ]
//   ezAnimGraphModelPoseOutputPin m_ModelPosePin; // [ property ]
// };
