#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <Foundation/Serialization/GraphPatch.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezMaterialResourceSlot, ezNoBase, 1, ezRTTIDefaultAllocator<ezMaterialResourceSlot>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new ezReadOnlyAttribute()),
    EZ_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new ezAssetBrowserAttribute("Material")),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetProperties, 2, ezRTTIDefaultAllocator<ezMeshAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("PrimitiveType", ezMeshPrimitive, m_PrimitiveType),
    EZ_ENUM_MEMBER_PROPERTY("ForwardDir", ezBasisAxis, m_ForwardDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::NegativeZ)),
    EZ_ENUM_MEMBER_PROPERTY("RightDir", ezBasisAxis, m_RightDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveX)),
    EZ_ENUM_MEMBER_PROPERTY("UpDir", ezBasisAxis, m_UpDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),
    EZ_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals)->AddAttributes(new ezDefaultValueAttribute(false)),
    EZ_MEMBER_PROPERTY("InvertNormals", m_bInvertNormals)->AddAttributes(new ezDefaultValueAttribute(false)),
    EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0001f, 10000.0f)),
    EZ_MEMBER_PROPERTY("NonUniformScaling", m_vNonUniformScaling)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.0001f), ezVec3(10000.0f))),
    EZ_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", "*.obj;*.fbx;*.ply;*.pbrt")), // todo. need to get this list of extensions automatically.
    EZ_MEMBER_PROPERTY("SubmeshName", m_sSubMeshName),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Radius2", m_fRadius2)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Detail", m_uiDetail)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 128)),
    EZ_MEMBER_PROPERTY("Detail2", m_uiDetail2)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 128)),
    EZ_MEMBER_PROPERTY("Cap", m_bCap)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Cap2", m_bCap2)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Angle", m_fAngle)->AddAttributes(new ezDefaultValueAttribute(360.0f), new ezClampValueAttribute(0.0f, 360.0f)),
    EZ_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("UseSubfolderForMaterialImport", m_bUseSubFolderForImportedMaterials)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new ezContainerAttribute(false, true, true)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

class ezMeshAssetPropertiesPatch_1_2 : public ezGraphPatch
{
public:
  ezMeshAssetPropertiesPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezMeshAssetProperties>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Primitive Type", "PrimitiveType");
    pNode->RenameProperty("Forward Dir", "ForwardDir");
    pNode->RenameProperty("Right Dir", "RightDir");
    pNode->RenameProperty("Up Dir", "UpDir");
    pNode->RenameProperty("Uniform Scaling", "UniformScaling");
    pNode->RenameProperty("Non-Uniform Scaling", "NonUniformScaling");
    pNode->RenameProperty("Mesh File", "MeshFile");
    pNode->RenameProperty("Submesh Name", "SubmeshName");
    pNode->RenameProperty("Radius 2", "Radius2");
    pNode->RenameProperty("Detail 2", "Detail2");
    pNode->RenameProperty("Cap 2", "Cap2");
    pNode->RenameProperty("Import Materials", "ImportMaterials");
    pNode->RenameProperty("Use Subfolder for Material Import", "UseSubfolderForMaterialImport");
  }
};

ezMeshAssetPropertiesPatch_1_2 g_MeshAssetPropertiesPatch_1_2;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezMeshPrimitive, 1)
  EZ_ENUM_CONSTANT(ezMeshPrimitive::File),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Box),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Rect),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Cylinder),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Cone),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Pyramid),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Sphere),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::HalfSphere),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::GeodesicSphere),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Capsule),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Torus),
EZ_END_STATIC_REFLECTED_ENUM();

ezMeshAssetProperties::ezMeshAssetProperties()
{
  m_uiVertices = 0;
  m_uiTriangles = 0;
  m_ForwardDir = ezBasisAxis::PositiveX;
  m_RightDir = ezBasisAxis::PositiveY;
  m_UpDir = ezBasisAxis::PositiveZ;
  m_fUniformScaling = 1.0f;
  m_fRadius = 0.5f;
  m_fRadius2 = 0.5f;
  m_fHeight = 1.0f;
  m_uiDetail = 1;
  m_uiDetail2 = 1;
  m_bCap = true;
  m_bCap2 = true;
  m_fAngle = 360.0f;
  m_bImportMaterials = true;
}


void ezMeshAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezRTTI::FindTypeByName("ezMeshAssetProperties"))
  {
    ezInt64 primType = e.m_pObject->GetTypeAccessor().GetValue("PrimitiveType").ConvertTo<ezInt64>();

    auto& props = *e.m_pPropertyStates;

    props["MeshFile"].m_Visibility = ezPropertyUiState::Invisible;
    props["Radius"].m_Visibility = ezPropertyUiState::Invisible;
    props["Radius2"].m_Visibility = ezPropertyUiState::Invisible;
    props["Height"].m_Visibility = ezPropertyUiState::Invisible;
    props["Detail"].m_Visibility = ezPropertyUiState::Invisible;
    props["Detail2"].m_Visibility = ezPropertyUiState::Invisible;
    props["Cap"].m_Visibility = ezPropertyUiState::Invisible;
    props["Cap2"].m_Visibility = ezPropertyUiState::Invisible;
    props["Angle"].m_Visibility = ezPropertyUiState::Invisible;
    props["ImportMaterials"].m_Visibility = ezPropertyUiState::Invisible;

    switch (primType)
    {
    case ezMeshPrimitive::File:
      props["MeshFile"].m_Visibility = ezPropertyUiState::Default;
      props["ImportMaterials"].m_Visibility = ezPropertyUiState::Default;
      break;

    case ezMeshPrimitive::Box:
    case ezMeshPrimitive::Rect:
      break;

    case ezMeshPrimitive::Capsule:
      props["Radius"].m_Visibility = ezPropertyUiState::Default;
      props["Height"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Detail2"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.Sphere.Detail1";
      props["Detail2"].m_sNewLabelText = "Prim.Sphere.Detail2";
      break;

    case ezMeshPrimitive::Cone:
      props["Radius"].m_Visibility = ezPropertyUiState::Default;
      props["Height"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Cap"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.Cylinder.Detail";
      break;

    case ezMeshPrimitive::Cylinder:
      props["Radius"].m_Visibility = ezPropertyUiState::Default;
      props["Radius2"].m_Visibility = ezPropertyUiState::Default;
      props["Height"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Cap"].m_Visibility = ezPropertyUiState::Default;
      props["Cap2"].m_Visibility = ezPropertyUiState::Default;
      props["Angle"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.Cylinder.Detail";
      props["Radius"].m_sNewLabelText = "Prim.Cylinder.Radius1";
      props["Radius2"].m_sNewLabelText = "Prim.Cylinder.Radius2";
      props["Angle"].m_sNewLabelText = "Prim.Cylinder.Angle";
      props["Cap"].m_sNewLabelText = "Prim.Cylinder.Cap1";
      props["Cap2"].m_sNewLabelText = "Prim.Cylinder.Cap2";
      break;

    case ezMeshPrimitive::GeodesicSphere:
      props["Radius"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.GeoSphere.Detail";
      break;

    case ezMeshPrimitive::HalfSphere:
      props["Radius"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Detail2"].m_Visibility = ezPropertyUiState::Default;
      props["Cap"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.Sphere.Detail1";
      props["Detail2"].m_sNewLabelText = "Prim.Sphere.Detail2";
      break;

    case ezMeshPrimitive::Pyramid:
      props["Cap"].m_Visibility = ezPropertyUiState::Default;
      break;

    case ezMeshPrimitive::Sphere:
      props["Radius"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Detail2"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.Sphere.Detail1";
      props["Detail2"].m_sNewLabelText = "Prim.Sphere.Detail2";
      break;

    case ezMeshPrimitive::Torus:
      props["Radius"].m_Visibility = ezPropertyUiState::Default;
      props["Radius2"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Detail2"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.Torus.Detail1";
      props["Detail2"].m_sNewLabelText = "Prim.Torus.Detail2";
      props["Radius"].m_sNewLabelText = "Prim.Torus.Radius1";
      props["Radius2"].m_sNewLabelText = "Prim.Torus.Radius2";
      break;
    }

  }
}

const ezString ezMeshAssetProperties::GetResourceSlotProperty(ezUInt32 uiSlot) const
{
  if (m_Slots.IsEmpty())
    return "";

  uiSlot %= m_Slots.GetCount();
  return m_Slots[uiSlot].m_sResource;
}


