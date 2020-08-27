#include <RendererCorePCH.h>

#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSkeletonJointGeometryType, 1)
EZ_ENUM_CONSTANTS(ezSkeletonJointGeometryType::None, ezSkeletonJointGeometryType::Capsule, ezSkeletonJointGeometryType::Sphere, ezSkeletonJointGeometryType::Box)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditableSkeletonJoint, 1, ezRTTIDefaultAllocator<ezEditableSkeletonJoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_MEMBER_PROPERTY("Transform", m_Transform)->AddFlags(ezPropertyFlags::Hidden)->AddAttributes(new ezDefaultValueAttribute(ezTransform::IdentityTransform())),
    EZ_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(ezPropertyFlags::PointerOwner | ezPropertyFlags::Hidden),
    EZ_ENUM_MEMBER_PROPERTY("Geometry", ezSkeletonJointGeometryType, m_Geometry),
    EZ_MEMBER_PROPERTY("Length", m_fLength),
    EZ_MEMBER_PROPERTY("Width", m_fWidth),
    EZ_MEMBER_PROPERTY("Thickness", m_fThickness),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditableSkeleton, 1, ezRTTIDefaultAllocator<ezEditableSkeleton>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("File", m_sAnimationFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", "*.fbx")),
    EZ_ENUM_MEMBER_PROPERTY("ForwardDir", ezBasisAxis, m_ForwardDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::NegativeZ)),
    EZ_ENUM_MEMBER_PROPERTY("RightDir", ezBasisAxis, m_RightDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveX)),
    EZ_ENUM_MEMBER_PROPERTY("UpDir", ezBasisAxis, m_UpDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),
    //EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0001f, 10000.0f)),

    EZ_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(ezPropertyFlags::PointerOwner | ezPropertyFlags::Hidden),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

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

static void AddChildJoints(ezSkeletonBuilder& sb, ezSkeletonResourceDescriptor* pDesc, const ezEditableSkeletonJoint* pParentJoint,
  const ezEditableSkeletonJoint* pJoint, ezUInt32 uiJointIdx)
{
  if (pDesc != nullptr && pJoint->m_Geometry != ezSkeletonJointGeometryType::None)
  {
    auto& geo = pDesc->m_Geometry.ExpandAndGetRef();
    geo.m_Type = pJoint->m_Geometry;
    geo.m_uiAttachedToJoint = uiJointIdx;
    geo.m_Transform.SetIdentity();
    geo.m_Transform.m_vScale.Set(pJoint->m_fLength, pJoint->m_fWidth, pJoint->m_fThickness);

    if (pParentJoint)
    {
      const float fBoneLength = (pParentJoint->m_Transform.m_vPosition - pJoint->m_Transform.m_vPosition).GetLength();
      // geo.m_Transform.m_vPosition.y = fBoneLength * 0.5f;
    }
  }

  for (const auto* pChildJoint : pJoint->m_Children)
  {
    const ezUInt32 idx = sb.AddJoint(pChildJoint->GetName(), pChildJoint->m_Transform, uiJointIdx);

    AddChildJoints(sb, pDesc, pJoint, pChildJoint, idx);
  }
}

void ezEditableSkeleton::GenerateSkeleton(ezSkeletonBuilder& sb, ezSkeletonResourceDescriptor* pDesc) const
{
  for (const auto* pJoint : m_Children)
  {
    const ezUInt32 idx = sb.AddJoint(pJoint->GetName(), pJoint->m_Transform);

    AddChildJoints(sb, pDesc, nullptr, pJoint, idx);
  }
}

void ezEditableSkeleton::GenerateSkeleton(ezSkeleton& skeleton, ezSkeletonResourceDescriptor* pDesc) const
{
  ezSkeletonBuilder sb;
  GenerateSkeleton(sb, pDesc);
  sb.BuildSkeleton(skeleton);
}

void ezEditableSkeleton::FillResourceDescriptor(ezSkeletonResourceDescriptor& desc) const
{
  desc.m_Geometry.Clear();
  GenerateSkeleton(desc.m_Skeleton, &desc);
}

static void BuildOzzRawSkeleton(const ezEditableSkeletonJoint& srcJoint, ozz::animation::offline::RawSkeleton::Joint& dstJoint)
{
  dstJoint.name = srcJoint.m_sName.GetString();
  dstJoint.transform.translation.x = srcJoint.m_Transform.m_vPosition.x;
  dstJoint.transform.translation.y = srcJoint.m_Transform.m_vPosition.y;
  dstJoint.transform.translation.z = srcJoint.m_Transform.m_vPosition.z;
  dstJoint.transform.rotation.x = srcJoint.m_Transform.m_qRotation.v.x;
  dstJoint.transform.rotation.y = srcJoint.m_Transform.m_qRotation.v.y;
  dstJoint.transform.rotation.z = srcJoint.m_Transform.m_qRotation.v.z;
  dstJoint.transform.rotation.w = srcJoint.m_Transform.m_qRotation.w;
  dstJoint.transform.scale.x = srcJoint.m_Transform.m_vScale.x;
  dstJoint.transform.scale.y = srcJoint.m_Transform.m_vScale.y;
  dstJoint.transform.scale.z = srcJoint.m_Transform.m_vScale.z;

  dstJoint.children.resize((size_t)srcJoint.m_Children.GetCount());

  for (ezUInt32 b = 0; b < srcJoint.m_Children.GetCount(); ++b)
  {
    BuildOzzRawSkeleton(*srcJoint.m_Children[b], dstJoint.children[b]);
  }
}

void ezEditableSkeleton::GenerateRawOzzSkeleton(ozz::animation::offline::RawSkeleton& out_Skeleton) const
{
  out_Skeleton.roots.resize((size_t)m_Children.GetCount());

  for (ezUInt32 b = 0; b < m_Children.GetCount(); ++b)
  {
    BuildOzzRawSkeleton(*m_Children[b], out_Skeleton.roots[b]);
  }
}

void ezEditableSkeleton::GenerateOzzSkeleton(ozz::animation::Skeleton& out_Skeleton) const
{
  ozz::animation::offline::RawSkeleton rawSkeleton;
  GenerateRawOzzSkeleton(rawSkeleton);

  ozz::animation::offline::SkeletonBuilder skeletonBuilder;
  auto pNewOzzSkeleton = skeletonBuilder(rawSkeleton);

  ezOzzUtils::CopySkeleton(&out_Skeleton, pNewOzzSkeleton.get());
}

ezEditableSkeletonJoint::ezEditableSkeletonJoint()
{
  m_Transform.SetIdentity();
}

ezEditableSkeletonJoint::~ezEditableSkeletonJoint()
{
  ClearJoints();
}

const char* ezEditableSkeletonJoint::GetName() const
{
  return m_sName.GetData();
}

void ezEditableSkeletonJoint::SetName(const char* sz)
{
  m_sName.Assign(sz);
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
  // do not copy:
  //  name
  //  transform
  //  children

  m_Geometry = pJoint->m_Geometry;
  m_fLength = pJoint->m_fLength;
  m_fWidth = pJoint->m_fWidth;
  m_fThickness = pJoint->m_fThickness;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_EditableSkeleton);
