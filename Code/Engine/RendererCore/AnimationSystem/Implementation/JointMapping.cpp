#include <PCH.h>

#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/JointMapping.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

ezArrayPtr<const ezJointMapping::Mapping> ezJointMapping::GetAllMappings() const
{
  return m_Mappings.GetArrayPtr();
}

void ezJointMapping::CreateMapping(const ezSkeleton& skeleton, const ezAnimationClipResourceDescriptor& animClip)
{
  const ezArrayMap<ezHashedString, ezUInt16>& nameToIndex = animClip.GetAllJointIndices();

  for (ezUInt32 i = 0; i < nameToIndex.GetCount(); ++i)
  {
    const ezUInt16 uiJointInSkeleton = skeleton.FindJointByName(nameToIndex.GetKey(i));
    if (uiJointInSkeleton != ezInvalidJointIndex)
    {
      Mapping& m = m_Mappings.ExpandAndGetRef();
      m.m_uiJointInSkeleton = uiJointInSkeleton;
      m.m_uiJointInAnimation = nameToIndex.GetValue(i);
    }
  }
}

void ezJointMapping::CreatePartialMapping(const ezSkeleton& skeleton, const ezAnimationClipResourceDescriptor& animClip,
                                          const ezTempHashedString& rootJoint)
{
  const ezUInt16 uiRootJointInSkeleton = skeleton.FindJointByName(rootJoint);
  if (uiRootJointInSkeleton == ezInvalidJointIndex)
    return;

  const ezArrayMap<ezHashedString, ezUInt16>& nameToIndex = animClip.GetAllJointIndices();

  for (ezUInt32 i = 0; i < nameToIndex.GetCount(); ++i)
  {
    const ezUInt16 uiJointInSkeleton = skeleton.FindJointByName(nameToIndex.GetKey(i));
    if (uiJointInSkeleton != ezInvalidJointIndex && skeleton.IsJointDescendantOf(uiJointInSkeleton, uiRootJointInSkeleton))
    {
      Mapping& m = m_Mappings.ExpandAndGetRef();
      m.m_uiJointInSkeleton = uiJointInSkeleton;
      m.m_uiJointInAnimation = nameToIndex.GetValue(i);
    }
  }
}
