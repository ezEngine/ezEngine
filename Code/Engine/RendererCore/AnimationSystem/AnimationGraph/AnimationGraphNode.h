#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

typedef ezTypedResourceHandle<class ezAnimationClipResource> ezAnimationClipResourceHandle;
typedef ezTypedResourceHandle<class ezSkeletonResource> ezSkeletonResourceHandle;

class EZ_RENDERERCORE_DLL ezAnimationGraphNode
{
public:
  ezAnimationGraphNode();
  virtual ~ezAnimationGraphNode();

  virtual void Step(ezTime tDiff);
  virtual bool Execute(const ezSkeleton& skeleton, ezAnimationPose& currentPose, ezTransform* pRootMotion) = 0;
};
