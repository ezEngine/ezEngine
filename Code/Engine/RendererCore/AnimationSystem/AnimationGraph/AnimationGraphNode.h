#pragma once

#include <Foundation/Time/Time.h>
#include <RendererCore/RendererCoreDLL.h>

class EZ_RENDERERCORE_DLL ezAnimationGraphNode
{
public:
  ezAnimationGraphNode();
  virtual ~ezAnimationGraphNode();

  virtual void Step(ezTime tDiff);
  //virtual bool Execute(const ezSkeleton& skeleton, ezAnimationPose& currentPose, ezTransform* pRootMotion) = 0;
};
