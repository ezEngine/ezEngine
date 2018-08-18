#include <PCH.h>

#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSkeletonJointGeometryType, 1)
EZ_ENUM_CONSTANTS(ezSkeletonJointGeometryType::None, ezSkeletonJointGeometryType::Capsule, ezSkeletonJointGeometryType::Sphere, ezSkeletonJointGeometryType::Box)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditableSkeletonJoint, 1, ezRTTIDefaultAllocator<ezEditableSkeletonJoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_MEMBER_PROPERTY("Transform", m_Transform)->AddFlags(ezPropertyFlags::Hidden)->AddAttributes(new ezDefaultValueAttribute(ezTransform::Identity())),
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
    EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0001f, 10000.0f)),

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

void ezEditableSkeleton::GenerateSkeleton(ezSkeletonBuilder& sb, ezSkeletonResourceDescriptor* pDesc) const
{
  auto AddChildJoints = [&sb, pDesc](auto& self, const ezEditableSkeletonJoint* pJoint, ezUInt32 uiJointIdx) -> void
  {
    if (pDesc != nullptr && pJoint->m_Geometry != ezSkeletonJointGeometryType::None)
    {
      auto& geo = pDesc->m_Geometry.ExpandAndGetRef();
      geo.m_Type = pJoint->m_Geometry;
      geo.m_uiAttachedToJoint = uiJointIdx;
      geo.m_Transform.SetIdentity();
      geo.m_Transform.m_vScale.Set(pJoint->m_fLength, pJoint->m_fWidth, pJoint->m_fThickness);
    }

    for (const auto* pChildJoint : pJoint->m_Children)
    {
      const ezUInt32 idx = sb.AddJoint(pChildJoint->GetName(), pChildJoint->m_Transform, uiJointIdx);

      self(self, pChildJoint, idx);
    }
  };

  for (const auto* pJoint : m_Children)
  {
    const ezUInt32 idx = sb.AddJoint(pJoint->GetName(), pJoint->m_Transform);

    AddChildJoints(AddChildJoints, pJoint, idx);
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
