#include <EditorPluginAssetsPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshAssetProperties, 2, ezRTTIDefaultAllocator<ezAnimatedMeshAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("ForwardDir", ezBasisAxis, m_ForwardDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::NegativeZ)),
    EZ_ENUM_MEMBER_PROPERTY("RightDir", ezBasisAxis, m_RightDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveX)),
    EZ_ENUM_MEMBER_PROPERTY("UpDir", ezBasisAxis, m_UpDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),
    EZ_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals)->AddAttributes(new ezDefaultValueAttribute(false)),
    EZ_MEMBER_PROPERTY("InvertNormals", m_bInvertNormals)->AddAttributes(new ezDefaultValueAttribute(false)),
    EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0001f, 10000.0f)),
    EZ_MEMBER_PROPERTY("Skeleton", m_sSkeletonFile)->AddAttributes(new ezAssetBrowserAttribute("Skeleton")),
    EZ_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", ".fbx;")),
    EZ_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("UseSubfolderForMaterialImport", m_bUseSubFolderForImportedMaterials)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new ezContainerAttribute(false, true, true)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimatedMeshAssetProperties::ezAnimatedMeshAssetProperties()
{
  m_ForwardDir = ezBasisAxis::PositiveX;
  m_RightDir = ezBasisAxis::PositiveY;
  m_UpDir = ezBasisAxis::PositiveZ;
  m_fUniformScaling = 1.0f;
  m_bImportMaterials = true;
}

