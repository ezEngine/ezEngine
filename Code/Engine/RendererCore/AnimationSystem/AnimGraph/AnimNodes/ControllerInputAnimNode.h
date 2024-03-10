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

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezControllerInputAnimNode

private:
  ezAnimGraphNumberOutputPin m_OutLeftStickX;   // [ property ]
  ezAnimGraphNumberOutputPin m_OutLeftStickY;   // [ property ]
  ezAnimGraphNumberOutputPin m_OutRightStickX;  // [ property ]
  ezAnimGraphNumberOutputPin m_OutRightStickY;  // [ property ]

  ezAnimGraphNumberOutputPin m_OutLeftTrigger;  // [ property ]
  ezAnimGraphNumberOutputPin m_OutRightTrigger; // [ property ]

  ezAnimGraphBoolOutputPin m_OutButtonA;        // [ property ]
  ezAnimGraphBoolOutputPin m_OutButtonB;        // [ property ]
  ezAnimGraphBoolOutputPin m_OutButtonX;        // [ property ]
  ezAnimGraphBoolOutputPin m_OutButtonY;        // [ property ]

  ezAnimGraphBoolOutputPin m_OutLeftShoulder;   // [ property ]
  ezAnimGraphBoolOutputPin m_OutRightShoulder;  // [ property ]

  ezAnimGraphBoolOutputPin m_OutPadLeft;        // [ property ]
  ezAnimGraphBoolOutputPin m_OutPadRight;       // [ property ]
  ezAnimGraphBoolOutputPin m_OutPadUp;          // [ property ]
  ezAnimGraphBoolOutputPin m_OutPadDown;        // [ property ]
};
