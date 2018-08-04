#include <PCH.h>

#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSkeletonBoneGeometryType, 1)
EZ_ENUM_CONSTANTS(ezSkeletonBoneGeometryType::None, ezSkeletonBoneGeometryType::Capsule, ezSkeletonBoneGeometryType::Sphere, ezSkeletonBoneGeometryType::Box)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditableSkeletonBone, 1, ezRTTIDefaultAllocator<ezEditableSkeletonBone>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_MEMBER_PROPERTY("Transform", m_Transform)->AddFlags(ezPropertyFlags::Hidden)->AddAttributes(new ezDefaultValueAttribute(ezTransform::Identity())),
    EZ_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(ezPropertyFlags::PointerOwner | ezPropertyFlags::Hidden),
    EZ_ENUM_MEMBER_PROPERTY("Geometry", ezSkeletonBoneGeometryType, m_Geometry),
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
  ClearBones();
}

void ezEditableSkeleton::ClearBones()
{
  for (ezEditableSkeletonBone* pChild : m_Children)
  {
    EZ_DEFAULT_DELETE(pChild);
  }

  m_Children.Clear();
}

void ezEditableSkeleton::FillResourceDescriptor(ezSkeletonResourceDescriptor& desc)
{
  desc.m_Geometry.Clear();

  // create the ezSkeleton
  {
    ezSkeletonBuilder sb;
    sb.SetSkinningMode(ezSkeleton::Mode::FourBones); // TODO: make this configurable ?

    auto AddChildBones = [&sb, &desc](auto& self, const ezEditableSkeletonBone* pBone, ezUInt32 uiBoneIdx) -> void {
      if (pBone->m_Geometry != ezSkeletonBoneGeometryType::None)
      {
        auto& geo = desc.m_Geometry.ExpandAndGetRef();
        geo.m_Type = pBone->m_Geometry;
        geo.m_uiAttachedToBone = uiBoneIdx;
        geo.m_Transform.SetIdentity();
        geo.m_Transform.m_vScale.Set(pBone->m_fLength, pBone->m_fWidth, pBone->m_fThickness);
      }

      for (const auto* pChildBone : pBone->m_Children)
      {
        const ezUInt32 idx = sb.AddBone(pChildBone->GetName(), pChildBone->m_Transform.GetAsMat4(), uiBoneIdx);

        self(self, pChildBone, idx);
      }
    };

    for (const auto* pBone : m_Children)
    {
      const ezUInt32 idx = sb.AddBone(pBone->GetName(), pBone->m_Transform.GetAsMat4());

      AddChildBones(AddChildBones, pBone, idx);
    }

    // TODO: a bit wasteful to allocate an object that is discarded right away
    desc.m_Skeleton = *sb.CreateSkeletonInstance();
  }
}

ezEditableSkeletonBone::ezEditableSkeletonBone()
{
  m_Transform.SetIdentity();
}

ezEditableSkeletonBone::~ezEditableSkeletonBone()
{
  ClearBones();
}

const char* ezEditableSkeletonBone::GetName() const
{
  return m_sName.GetData();
}

void ezEditableSkeletonBone::SetName(const char* sz)
{
  m_sName.Assign(sz);
}

void ezEditableSkeletonBone::ClearBones()
{
  for (ezEditableSkeletonBone* pChild : m_Children)
  {
    EZ_DEFAULT_DELETE(pChild);
  }

  m_Children.Clear();
}

void ezEditableSkeletonBone::CopyPropertiesFrom(const ezEditableSkeletonBone* pBone)
{
  // do not copy:
  //  name
  //  transform
  //  children

  m_Geometry = pBone->m_Geometry;
  m_fLength = pBone->m_fLength;
  m_fWidth = pBone->m_fWidth;
  m_fThickness = pBone->m_fThickness;
}
