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

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

private:
  ezAnimGraphTriggerOutputPin m_ButtonA; // [ property ]
  ezAnimGraphTriggerOutputPin m_ButtonB; // [ property ]
  ezAnimGraphTriggerOutputPin m_ButtonX; // [ property ]
  ezAnimGraphTriggerOutputPin m_ButtonY; // [ property ]

  ezAnimGraphNumberOutputPin m_LeftStickX;  // [ property ]
  ezAnimGraphNumberOutputPin m_LeftStickY;  // [ property ]
  ezAnimGraphNumberOutputPin m_RightStickX; // [ property ]
  ezAnimGraphNumberOutputPin m_RightStickY; // [ property ]

  ezAnimGraphNumberOutputPin m_LeftTrigger;  // [ property ]
  ezAnimGraphNumberOutputPin m_RightTrigger; // [ property ]

  ezAnimGraphTriggerOutputPin m_LeftShoulder;  // [ property ]
  ezAnimGraphTriggerOutputPin m_RightShoulder; // [ property ]

  ezAnimGraphTriggerOutputPin m_PadLeft;  // [ property ]
  ezAnimGraphTriggerOutputPin m_PadRight; // [ property ]
  ezAnimGraphTriggerOutputPin m_PadUp;    // [ property ]
  ezAnimGraphTriggerOutputPin m_PadDown;  // [ property ]
};
