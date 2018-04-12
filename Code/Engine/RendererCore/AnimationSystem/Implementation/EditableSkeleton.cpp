
#include <PCH.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditableSkeletonBone, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_ARRAY_MEMBER_PROPERTY/*_READ_ONLY*/("Children", m_Children)->AddFlags(ezPropertyFlags::PointerOwner/* | ezPropertyFlags::Hidden*/),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditableSkeleton, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("File", m_sAnimationFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", "*.fbx")),
    EZ_ENUM_MEMBER_PROPERTY("ForwardDir", ezBasisAxis, m_ForwardDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::NegativeZ)),
    EZ_ENUM_MEMBER_PROPERTY("RightDir", ezBasisAxis, m_RightDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveX)),
    EZ_ENUM_MEMBER_PROPERTY("UpDir", ezBasisAxis, m_UpDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),

    EZ_ARRAY_MEMBER_PROPERTY/*_READ_ONLY*/("Children", m_Children)->AddFlags(ezPropertyFlags::PointerOwner /*| ezPropertyFlags::Hidden*/),
  }
    EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezEditableSkeleton::ezEditableSkeleton() = default;
ezEditableSkeleton::~ezEditableSkeleton() = default;

ezEditableSkeletonBone::ezEditableSkeletonBone() = default;
ezEditableSkeletonBone::~ezEditableSkeletonBone() = default;

const char* ezEditableSkeletonBone::GetName() const
{
  return m_sName.GetData();
}

void ezEditableSkeletonBone::SetName(const char* sz)
{
  m_sName.Assign(sz);
}
