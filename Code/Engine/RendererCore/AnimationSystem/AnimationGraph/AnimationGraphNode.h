#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

typedef ezTypedResourceHandle<class ezAnimationClipResource> ezAnimationClipResourceHandle;
typedef ezTypedResourceHandle<class ezSkeletonResource> ezSkeletonResourceHandle;

class EZ_RENDERERCORE_DLL ezAnimationGraphNode
{
public:
  ezAnimationGraphNode();
  ~ezAnimationGraphNode();

  virtual void Step(ezTime tDiff);
  virtual void Execute(const ezSkeleton& skeleton, ezAnimationPose& currentPose) = 0;

};
