#include <RendererCore/RendererCorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSkeletonJointGeometryType, 1)
EZ_ENUM_CONSTANTS(ezSkeletonJointGeometryType::None, ezSkeletonJointGeometryType::Capsule, ezSkeletonJointGeometryType::CapsuleSideways, ezSkeletonJointGeometryType::Sphere, ezSkeletonJointGeometryType::Box)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditableSkeletonBoneShape, 1, ezRTTIDefaultAllocator<ezEditableSkeletonBoneShape>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Geometry", ezSkeletonJointGeometryType, m_Geometry),
    EZ_MEMBER_PROPERTY("Offset", m_vOffset),
    EZ_MEMBER_PROPERTY("Rotation", m_qRotation),
    EZ_MEMBER_PROPERTY("Length", m_fLength)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0.01f, 10.0f)),
    EZ_MEMBER_PROPERTY("Width", m_fWidth)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.01f, 10.0f)),
    EZ_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.01f, 10.0f)),

  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditableSkeletonBoneCollider, 1, ezRTTIDefaultAllocator<ezEditableSkeletonBoneCollider>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Identifier", m_sIdentifier)->AddAttributes(new ezHiddenAttribute()),
    EZ_ARRAY_MEMBER_PROPERTY("VertexPositions", m_VertexPositions)->AddAttributes(new ezHiddenAttribute()),
    EZ_ARRAY_MEMBER_PROPERTY("TriangleIndices", m_TriangleIndices)->AddAttributes(new ezHiddenAttribute()),

  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditableSkeletonJoint, 2, ezRTTIDefaultAllocator<ezEditableSkeletonJoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName)->AddAttributes(new ezReadOnlyAttribute()),
    EZ_MEMBER_PROPERTY("Transform", m_LocalTransform)->AddFlags(ezPropertyFlags::Hidden)->AddAttributes(new ezDefaultValueAttribute(ezTransform::MakeIdentity())),
    EZ_MEMBER_PROPERTY_READ_ONLY("GizmoOffsetTranslationRO", m_vGizmoOffsetPositionRO)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY_READ_ONLY("GizmoOffsetRotationRO", m_qGizmoOffsetRotationRO)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("LocalRotation", m_qLocalJointRotation),
    EZ_ENUM_MEMBER_PROPERTY("JointType", ezSkeletonJointType, m_JointType),
    EZ_MEMBER_PROPERTY("Stiffness", m_fStiffness)->AddAttributes(new ezDefaultValueAttribute(10.0f)),
    EZ_MEMBER_PROPERTY("SwingLimitY", m_SwingLimitY)->AddAttributes(new ezClampValueAttribute(ezAngle(), ezAngle::MakeFromDegree(170)), new ezDefaultValueAttribute(ezAngle::MakeFromDegree(30))),
    EZ_MEMBER_PROPERTY("SwingLimitZ", m_SwingLimitZ)->AddAttributes(new ezClampValueAttribute(ezAngle(), ezAngle::MakeFromDegree(170)), new ezDefaultValueAttribute(ezAngle::MakeFromDegree(30))),
    EZ_MEMBER_PROPERTY("TwistLimitHalfAngle", m_TwistLimitHalfAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::MakeFromDegree(10), ezAngle::MakeFromDegree(170)), new ezDefaultValueAttribute(ezAngle::MakeFromDegree(30))),
    EZ_MEMBER_PROPERTY("TwistLimitCenterAngle", m_TwistLimitCenterAngle),
    EZ_MEMBER_PROPERTY("OverrideSurface", m_bOverrideSurface),
    EZ_MEMBER_PROPERTY("Surface", m_sSurfaceOverride)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("OverrideCollisionLayer", m_bOverrideCollisionLayer),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayerOverride)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),

    EZ_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(ezPropertyFlags::PointerOwner | ezPropertyFlags::Hidden),
    EZ_ARRAY_MEMBER_PROPERTY("BoneShapes", m_BoneShapes),
    EZ_ARRAY_MEMBER_PROPERTY("Colliders", m_BoneColliders)->AddAttributes(new ezContainerAttribute(false, false, false)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTransformManipulatorAttribute(nullptr, "LocalRotation", nullptr, "GizmoOffsetTranslationRO", "GizmoOffsetRotationRO"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditableSkeleton, 2, ezRTTIDefaultAllocator<ezEditableSkeleton>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("File", m_sSourceFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", ezFileBrowserAttribute::MeshesWithAnimations)),
    EZ_ENUM_MEMBER_PROPERTY("ImportTransform", ezMeshImportTransform, m_ImportTransform),
    EZ_ENUM_MEMBER_PROPERTY("RightDir", ezBasisAxis, m_RightDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::NegativeX)),
    EZ_ENUM_MEMBER_PROPERTY("UpDir", ezBasisAxis, m_UpDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),
    EZ_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0001f, 10000.0f)),
    EZ_ENUM_MEMBER_PROPERTY("BoneDirection", ezBasisAxis, m_BoneDirection)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),
    EZ_MEMBER_PROPERTY("PreviewMesh", m_sPreviewMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned", ezDependencyFlags::None)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("Surface", m_sSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("MaxImpulse", m_fMaxImpulse)->AddAttributes(new ezDefaultValueAttribute(100.f)),
    EZ_MEMBER_PROPERTY("LeftFootJoint", m_sLeftFootJoint),
    EZ_MEMBER_PROPERTY("RightFootJoint", m_sRightFootJoint),

    EZ_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(ezPropertyFlags::PointerOwner | ezPropertyFlags::Hidden),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezExposedBone, ezNoBase, 1, ezRTTIDefaultAllocator<ezExposedBone>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Parent", m_sParent),
    EZ_MEMBER_PROPERTY("Transform", m_Transform),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezExposedBone);
// clang-format on


void operator<<(ezStreamWriter& inout_stream, const ezExposedBone& bone)
{
  inout_stream << bone.m_sName;
  inout_stream << bone.m_sParent;
  inout_stream << bone.m_Transform;
}

void operator>>(ezStreamReader& inout_stream, ezExposedBone& ref_bone)
{
  inout_stream >> ref_bone.m_sName;
  inout_stream >> ref_bone.m_sParent;
  inout_stream >> ref_bone.m_Transform;
}

bool operator==(const ezExposedBone& lhs, const ezExposedBone& rhs)
{
  if (lhs.m_sName != rhs.m_sName)
    return false;
  if (lhs.m_sParent != rhs.m_sParent)
    return false;
  if (lhs.m_Transform != rhs.m_Transform)
    return false;
  return true;
}

ezEditableSkeleton::ezEditableSkeleton() = default;
ezEditableSkeleton::~ezEditableSkeleton()
{
  ClearJoints();
}

void ezEditableSkeleton::ClearJoints()
{
  for (ezEditableSkeletonJoint* pChild : m_Children)
  {
    EZ_DEFAULT_DELETE(pChild);
  }

  m_Children.Clear();
}

void ezEditableSkeleton::CreateJointsRecursive(ezSkeletonBuilder& ref_sb, ezSkeletonResourceDescriptor& ref_desc, const ezEditableSkeletonJoint* pParentJoint, const ezEditableSkeletonJoint* pThisJoint, ezUInt16 uiThisJointIdx, const ezQuat& qParentAccuRot, const ezMat4& mRootTransform) const
{
  for (auto& shape : pThisJoint->m_BoneShapes)
  {
    auto& geo = ref_desc.m_Geometry.ExpandAndGetRef();

    geo.m_Type = shape.m_Geometry;
    geo.m_uiAttachedToJoint = static_cast<ezUInt16>(uiThisJointIdx);
    geo.m_Transform.SetIdentity();
    geo.m_Transform.m_vScale.Set(shape.m_fLength, shape.m_fWidth, shape.m_fThickness);
    geo.m_Transform.m_vPosition = shape.m_vOffset;
    geo.m_Transform.m_qRotation = shape.m_qRotation;
  }

  for (auto& shape : pThisJoint->m_BoneColliders)
  {
    auto& geo = ref_desc.m_Geometry.ExpandAndGetRef();
    geo.m_Type = ezSkeletonJointGeometryType::ConvexMesh;
    geo.m_uiAttachedToJoint = static_cast<ezUInt16>(uiThisJointIdx);
    geo.m_Transform.SetIdentity();
    geo.m_VertexPositions = shape.m_VertexPositions;
    geo.m_TriangleIndices = shape.m_TriangleIndices;
  }

  const ezVec3 s = pThisJoint->m_LocalTransform.m_vScale;
  if (!s.IsEqual(ezVec3(1), 0.1f))
  {
    // ezLog::Warning("Mesh bone '{}' has scaling values of {}/{}/{} - this is not supported.", pThisJoint->m_sName, s.x, s.y, s.z);
  }

  const ezQuat qThisAccuRot = qParentAccuRot * pThisJoint->m_LocalTransform.m_qRotation;
  ezQuat qParentGlobalRot;

  {
    // as always, the root transform is the bane of my existence
    // since it can contain mirroring, the final global rotation of a joint will be incorrect if we don't incorporate the root scale
    // unfortunately this can't be done once for the first node, but has to be done on the result instead

    ezMat4 full;
    ezMsgAnimationPoseUpdated::ComputeFullBoneTransform(mRootTransform, qParentAccuRot.GetAsMat4(), full, qParentGlobalRot);
  }

  ref_sb.SetJointLimit(uiThisJointIdx, pThisJoint->m_qLocalJointRotation, pThisJoint->m_JointType, pThisJoint->m_SwingLimitY, pThisJoint->m_SwingLimitZ, pThisJoint->m_TwistLimitHalfAngle, pThisJoint->m_TwistLimitCenterAngle, pThisJoint->m_fStiffness);

  ref_sb.SetJointCollisionLayer(uiThisJointIdx, pThisJoint->m_bOverrideCollisionLayer ? pThisJoint->m_uiCollisionLayerOverride : m_uiCollisionLayer);
  ref_sb.SetJointSurface(uiThisJointIdx, pThisJoint->m_bOverrideSurface ? pThisJoint->m_sSurfaceOverride : m_sSurfaceFile);

  for (const auto* pChildJoint : pThisJoint->m_Children)
  {
    const ezUInt16 uiChildJointIdx = ref_sb.AddJoint(pChildJoint->GetName(), pChildJoint->m_LocalTransform, uiThisJointIdx);

    CreateJointsRecursive(ref_sb, ref_desc, pThisJoint, pChildJoint, uiChildJointIdx, qThisAccuRot, mRootTransform);
  }
}

void ezEditableSkeleton::FillResourceDescriptor(ezSkeletonResourceDescriptor& ref_desc) const
{
  ref_desc.m_fMaxImpulse = m_fMaxImpulse;
  ref_desc.m_Geometry.Clear();

  ezSkeletonBuilder sb;
  for (const auto* pJoint : m_Children)
  {
    const ezUInt16 idx = sb.AddJoint(pJoint->GetName(), pJoint->m_LocalTransform);

    CreateJointsRecursive(sb, ref_desc, nullptr, pJoint, idx, ezQuat::MakeIdentity(), ref_desc.m_RootTransform.GetAsMat4());
  }

  sb.BuildSkeleton(ref_desc.m_Skeleton);
  ref_desc.m_Skeleton.m_BoneDirection = m_BoneDirection;

  ref_desc.m_uiLeftFootJoint = ref_desc.m_Skeleton.FindJointByName(ezTempHashedString(m_sLeftFootJoint));
  ref_desc.m_uiRightFootJoint = ref_desc.m_Skeleton.FindJointByName(ezTempHashedString(m_sRightFootJoint));
}

static void BuildOzzRawSkeleton(const ezEditableSkeletonJoint& srcJoint, ozz::animation::offline::RawSkeleton::Joint& ref_dstJoint)
{
  ref_dstJoint.name = srcJoint.m_sName.GetString();
  ref_dstJoint.transform.translation.x = srcJoint.m_LocalTransform.m_vPosition.x;
  ref_dstJoint.transform.translation.y = srcJoint.m_LocalTransform.m_vPosition.y;
  ref_dstJoint.transform.translation.z = srcJoint.m_LocalTransform.m_vPosition.z;
  ref_dstJoint.transform.rotation.x = srcJoint.m_LocalTransform.m_qRotation.x;
  ref_dstJoint.transform.rotation.y = srcJoint.m_LocalTransform.m_qRotation.y;
  ref_dstJoint.transform.rotation.z = srcJoint.m_LocalTransform.m_qRotation.z;
  ref_dstJoint.transform.rotation.w = srcJoint.m_LocalTransform.m_qRotation.w;
  ref_dstJoint.transform.scale.x = srcJoint.m_LocalTransform.m_vScale.x;
  ref_dstJoint.transform.scale.y = srcJoint.m_LocalTransform.m_vScale.y;
  ref_dstJoint.transform.scale.z = srcJoint.m_LocalTransform.m_vScale.z;

  ref_dstJoint.children.resize((size_t)srcJoint.m_Children.GetCount());

  for (ezUInt32 b = 0; b < srcJoint.m_Children.GetCount(); ++b)
  {
    BuildOzzRawSkeleton(*srcJoint.m_Children[b], ref_dstJoint.children[b]);
  }
}

void ezEditableSkeleton::GenerateRawOzzSkeleton(ozz::animation::offline::RawSkeleton& out_skeleton) const
{
  out_skeleton.roots.resize((size_t)m_Children.GetCount());

  for (ezUInt32 b = 0; b < m_Children.GetCount(); ++b)
  {
    BuildOzzRawSkeleton(*m_Children[b], out_skeleton.roots[b]);
  }
}

void ezEditableSkeleton::GenerateOzzSkeleton(ozz::animation::Skeleton& out_skeleton) const
{
  ozz::animation::offline::RawSkeleton rawSkeleton;
  GenerateRawOzzSkeleton(rawSkeleton);

  ozz::animation::offline::SkeletonBuilder skeletonBuilder;
  auto pNewOzzSkeleton = skeletonBuilder(rawSkeleton);

  ezOzzUtils::CopySkeleton(&out_skeleton, pNewOzzSkeleton.get());
}

ezEditableSkeletonJoint::ezEditableSkeletonJoint() = default;

ezEditableSkeletonJoint::~ezEditableSkeletonJoint()
{
  ClearJoints();
}

const char* ezEditableSkeletonJoint::GetName() const
{
  return m_sName.GetData();
}

void ezEditableSkeletonJoint::SetName(const char* szSz)
{
  m_sName.Assign(szSz);
}

void ezEditableSkeletonJoint::ClearJoints()
{
  for (ezEditableSkeletonJoint* pChild : m_Children)
  {
    EZ_DEFAULT_DELETE(pChild);
  }
  m_Children.Clear();
}

void ezEditableSkeletonJoint::CopyPropertiesFrom(const ezEditableSkeletonJoint* pJoint)
{
  // copy existing (user edited) properties from pJoint into this joint
  // which has just been imported from file

  // do not copy:
  //  name
  //  transform
  //  children
  //  bone collider geometry (vertices, indices)

  // synchronize user config of bone colliders
  for (ezUInt32 i = 0; i < m_BoneColliders.GetCount(); ++i)
  {
    auto& dst = m_BoneColliders[i];

    for (ezUInt32 j = 0; j < pJoint->m_BoneColliders.GetCount(); ++j)
    {
      const auto& src = pJoint->m_BoneColliders[j];

      if (dst.m_sIdentifier == src.m_sIdentifier)
      {
        // dst.m_bOverrideSurface = src.m_bOverrideSurface;
        // dst.m_bOverrideCollisionLayer = src.m_bOverrideCollisionLayer;
        // dst.m_sSurfaceOverride = src.m_sSurfaceOverride;
        // dst.m_uiCollisionLayerOverride = src.m_uiCollisionLayerOverride;
        break;
      }
    }
  }

  m_BoneShapes = pJoint->m_BoneShapes;
  m_qLocalJointRotation = pJoint->m_qLocalJointRotation;
  m_JointType = pJoint->m_JointType;
  m_SwingLimitY = pJoint->m_SwingLimitY;
  m_SwingLimitZ = pJoint->m_SwingLimitZ;
  m_TwistLimitHalfAngle = pJoint->m_TwistLimitHalfAngle;
  m_TwistLimitCenterAngle = pJoint->m_TwistLimitCenterAngle;
  m_fStiffness = pJoint->m_fStiffness;

  m_bOverrideSurface = pJoint->m_bOverrideSurface;
  m_bOverrideCollisionLayer = pJoint->m_bOverrideCollisionLayer;
  m_sSurfaceOverride = pJoint->m_sSurfaceOverride;
  m_uiCollisionLayerOverride = pJoint->m_uiCollisionLayerOverride;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_EditableSkeleton);
