#include <PCH.h>

#include <RendererCore/AnimationSystem/AnimationGraph/AnimationGraphNode.h>

ezAnimationGraphNode::ezAnimationGraphNode() = default;
ezAnimationGraphNode::~ezAnimationGraphNode() = default;

void ezAnimationGraphNode::Step(ezTime tDiff) {}



EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimationGraph_Implementation_AnimationGraphNode);

