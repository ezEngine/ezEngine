#include <PCH.h>

#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Debug/DebugRenderer.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgAnimationPoseUpdated);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgAnimationPoseUpdated, 1, ezRTTIDefaultAllocator<ezMsgAnimationPoseUpdated>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

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

  SetToBindPoseInLocalSpace(skeleton);
}

void ezAnimationPose::SetToBindPoseInLocalSpace(const ezSkeleton& skeleton)
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
void ezAnimationPose::ConvertFromLocalSpaceToObjectSpace(const ezSkeleton& skeleton)
{
  // TODO: store current space and assert that it is correct ?

  const ezUInt32 numTransforms = GetTransformCount();

  EZ_ASSERT_DEV(skeleton.GetJointCount() == numTransforms, "Pose and skeleton have different joint count!");

  // STEP 1: convert pose matrices from local (joint) space to object (skeleton) space by concatenating parent transforms

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
}

void ezAnimationPose::ConvertFromObjectSpaceToSkinningSpace(const ezSkeleton& skeleton)
{
  // TODO: store current space and assert that it is correct ?

  // STEP 2: multiply each joint's individual inverse-global-pose matrix into the result

  const ezUInt32 numTransforms = GetTransformCount();

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

void ezAnimationPose::VisualizePose(const ezDebugRendererContext& context, const ezSkeleton& skeleton, const ezTransform& objectTransform,
                                    ezUInt16 uiStartJoint) const
{
  // TODO: store current space and assert that it is correct ?

  ezHybridArray<ezDebugRenderer::Line, 128> lines;

  const ezUInt16 numJoints = skeleton.GetJointCount();
  for (ezUInt16 thisJointIdx = 0; thisJointIdx < numJoints; ++thisJointIdx)
  {
    const ezSkeletonJoint& thisJoint = skeleton.GetJointByIndex(thisJointIdx);

    if (uiStartJoint != ezInvalidJointIndex)
    {
      ezUInt16 uiParentIdx = thisJointIdx;
      while (uiParentIdx != ezInvalidJointIndex)
      {
        if (uiParentIdx == uiStartJoint)
          goto render;

        uiParentIdx = skeleton.GetJointByIndex(uiParentIdx).GetParentIndex();
      }

      // parent joint not found in hierarchy -> skip this one
      continue;
    }

  render:

    ezTransform thisJointTransform;
    thisJointTransform.SetFromMat4(GetTransform(thisJointIdx));
    thisJointTransform = objectTransform * thisJointTransform;

    if (thisJoint.IsRootJoint() || thisJointIdx == uiStartJoint)
    {
      const ezBoundingSphere sphere(ezVec3::ZeroVector(), 0.05f);
      thisJointTransform.m_vScale.Set(1);
      ezDebugRenderer::DrawLineSphere(context, sphere, ezColor::MediumVioletRed, thisJointTransform);
    }
    else
    {
      const ezUInt16 parentJointIdx = thisJoint.GetParentIndex();
      const ezSkeletonJoint& parentJoint = skeleton.GetJointByIndex(parentJointIdx);
      ezTransform parentJointTransform;
      parentJointTransform.SetFromMat4(GetTransform(parentJointIdx));
      parentJointTransform = objectTransform * parentJointTransform;

      thisJointTransform.m_vScale.Set(1);

      auto& line = lines.ExpandAndGetRef();
      line.m_start = thisJointTransform.m_vPosition;
      line.m_end = parentJointTransform.m_vPosition;

      const ezBoundingSphere sphere(ezVec3::ZeroVector(), (line.m_start - line.m_end).GetLength() * 0.2f);
      ezDebugRenderer::DrawLineSphere(context, sphere, ezColor::Yellow, thisJointTransform);
    }
  }

  ezDebugRenderer::DrawLines(context, lines, ezColor::GreenYellow);
}

void ezAnimationPose::SetTransform(ezUInt16 uiIndex, const ezMat4& transform)
{
  m_Transforms[uiIndex] = transform;
  m_TransformsValid.SetBit(uiIndex);
}

void ezAnimationPose::SetTransformValid(ezUInt16 uiIndex, bool bValid)
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
