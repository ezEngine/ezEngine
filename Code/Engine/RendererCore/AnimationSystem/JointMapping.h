#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <RendererCore/AnimationSystem/Declarations.h>

class ezSkeleton;

class ezJointMapping
{
public:
  struct Mapping
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt16 m_uiJointInSkeleton;
    ezUInt16 m_uiJointInAnimation;
  };

  ezArrayPtr<const Mapping> GetAllMappings() const;

  void CreateMapping(const ezSkeleton& skeleton, const ezAnimationClipResourceDescriptor& animClip);
  void CreatePartialMapping(const ezSkeleton& skeleton, const ezAnimationClipResourceDescriptor& animClip, const ezTempHashedString& rootJoint);

private:
  ezDynamicArray<Mapping> m_Mappings;
};

