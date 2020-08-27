#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <Foundation/Time/Time.h>

class EZ_RENDERERCORE_DLL ezAnimationGraphNode
{
public:
  ezAnimationGraphNode();
  virtual ~ezAnimationGraphNode();

  virtual void Step(ezTime tDiff);
  //virtual bool Execute(const ezSkeleton& skeleton, ezAnimationPose& currentPose, ezTransform* pRootMotion) = 0;
};
