#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezControllerInputAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezControllerInputAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph* pOwner, ezTime tDiff, const ezSkeletonResource* pSkeleton) override;

private:
  ezAnimGraphTriggerOutputPin m_ButtonA; // [ property ]
  ezAnimGraphTriggerOutputPin m_ButtonB; // [ property ]
  ezAnimGraphTriggerOutputPin m_ButtonX; // [ property ]
  ezAnimGraphTriggerOutputPin m_ButtonY; // [ property ]

  ezAnimGraphTriggerOutputPin m_StickLeft;  // [ property ]
  ezAnimGraphTriggerOutputPin m_StickRight; // [ property ]
  ezAnimGraphTriggerOutputPin m_StickUp;    // [ property ]
  ezAnimGraphTriggerOutputPin m_StickDown;  // [ property ]
};
