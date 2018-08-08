#include <PCH.h>

#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

 ezAnimationPose::ezAnimationPose() = default;
 ezAnimationPose::~ezAnimationPose() = default;

 void ezAnimationPose::Configure(const ezSkeleton& skeleton)
{
  EZ_ASSERT_DEV(skeleton.GetJointCount() > 0, "Animation pose needs a valid skeleton which also has at least one joint!");

  // Allocate storage once for matrices and validity bits
  m_Transforms.SetCountUninitialized(skeleton.GetJointCount());

  // By default all transforms are invalid.
  m_TransformsValid.SetCount(skeleton.GetJointCount());
  m_TransformsValid.ClearAllBits();

  SetToBindPose(skeleton);
}

void ezAnimationPose::SetToBindPose(const ezSkeleton& skeleton)
{
  EZ_ASSERT_DEV(skeleton.GetJointCount() == GetTransformCount(), "Pose and skeleton have different joint count!");

  // TODO: Check additional compatibility of pose object with this skeleton?

  // Copy bind pose to pose by using the initial joint transforms of the skeleton.
  const ezUInt32 numTransforms = m_Transforms.GetCount();
  for (ezUInt32 i = 0; i < numTransforms; ++i)
  {
    m_Transforms[i] = skeleton.GetJointByIndex(i).GetBindPoseLocalTransform().GetAsMat4();
  }
}

void ezAnimationPose::CalculateObjectSpaceTransforms(const ezSkeleton& skeleton)
{
  const ezUInt32 numTransforms = GetTransformCount();

  EZ_ASSERT_DEV(skeleton.GetJointCount() == numTransforms, "Pose and skeleton have different joint count!");

  // STEP 1: convert pose matrices from local space to global space by concatenating parent transforms
  // STEP 2: multiply each joint's individual inverse-global-pose matrix into the result
  // this should (theoretically) first move the vertices into "joint space" such that afterwards the global skeleton transform
  // moves it back into the animated global space

  // Since the joints are sorted (at least no child joint comes before it's parent joint)
  // we can simply grab the already stored parent transform from the pose to get the multiplied
  // transforms up to the child joint we currently work on.
  for (ezUInt32 i = 0; i < numTransforms; ++i)
  {
    const ezSkeletonJoint& joint = skeleton.GetJointByIndex(i);

    // If it is a root joint the transform is already final.
    if (!joint.IsRootJoint())
    {
      // else grab transform of parent joint and use it to make the final transform for this joint
      m_Transforms[i] = m_Transforms[joint.GetParentIndex()] * m_Transforms[i];
    }
  }

  for (ezUInt32 i = 0; i < numTransforms; ++i)
  {
    const ezSkeletonJoint& joint = skeleton.GetJointByIndex(i);
    m_Transforms[i] = m_Transforms[i] * joint.GetInverseBindPoseGlobalTransform().GetAsMat4();
  }
}

ezVec3 ezAnimationPose::SkinPositionWithSingleJoint(const ezVec3& Position, ezUInt32 uiIndex) const
{
  return m_Transforms[uiIndex].TransformPosition(Position);
}

ezVec3 ezAnimationPose::SkinPositionWithFourJoints(const ezVec3& Position, const ezVec4U32& indices, const ezVec4& weights) const
{
  ezVec3 Pos1 = m_Transforms[indices.x].TransformPosition(Position);
  ezVec3 Pos2 = m_Transforms[indices.y].TransformPosition(Position);
  ezVec3 Pos3 = m_Transforms[indices.z].TransformPosition(Position);
  ezVec3 Pos4 = m_Transforms[indices.w].TransformPosition(Position);

  return (Pos1 * weights.x) + (Pos2 * weights.y) + (Pos3 * weights.z) + (Pos4 * weights.w);
}

ezVec3 ezAnimationPose::SkinDirectionWithSingleJoint(const ezVec3& Direction, ezUInt32 uiIndex) const
{
  return m_Transforms[uiIndex].TransformDirection(Direction);
}

ezVec3 ezAnimationPose::SkinDirectionWithFourJoints(const ezVec3& Direction, const ezVec4U32& indices, const ezVec4& weights) const
{
  ezVec3 Dir1 = m_Transforms[indices.x].TransformDirection(Direction);
  ezVec3 Dir2 = m_Transforms[indices.y].TransformDirection(Direction);
  ezVec3 Dir3 = m_Transforms[indices.z].TransformDirection(Direction);
  ezVec3 Dir4 = m_Transforms[indices.w].TransformDirection(Direction);

  return (Dir1 * weights.x) + (Dir2 * weights.y) + (Dir3 * weights.z) + (Dir4 * weights.w);
}

void ezAnimationPose::SetTransform(ezUInt32 uiIndex, const ezMat4& transform)
{
  m_Transforms[uiIndex] = transform;
  m_TransformsValid.SetBit(uiIndex);
}

void ezAnimationPose::SetTransformValid(ezUInt32 uiIndex, bool bValid)
{
  if (bValid)
  {
    m_TransformsValid.SetBit(uiIndex);
  }
  else
  {
    m_TransformsValid.ClearBit(uiIndex);
  }
}

void ezAnimationPose::SetValidityOfAllTransforms(bool bValid)
{
  if (bValid)
  {
    m_TransformsValid.SetAllBits();
  }
  else
  {
    m_TransformsValid.ClearAllBits();
  }
}
