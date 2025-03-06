#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetProperties, 4, ezRTTIDefaultAllocator<ezMeshAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("PrimitiveType", ezMeshPrimitive, m_PrimitiveType),
    EZ_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", ezFileBrowserAttribute::Meshes)),
    EZ_ENUM_MEMBER_PROPERTY("ImportTransform", ezMeshImportTransform, m_ImportTransform),
    EZ_ENUM_MEMBER_PROPERTY("RightDir", ezBasisAxis, m_RightDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::NegativeX)),
    EZ_ENUM_MEMBER_PROPERTY("UpDir", ezBasisAxis, m_UpDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),
    EZ_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0001f, 10000.0f)),
    EZ_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals),
    EZ_MEMBER_PROPERTY("RecalculateTangents", m_bRecalculateTrangents)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ENUM_MEMBER_PROPERTY("NormalPrecision", ezMeshNormalPrecision, m_NormalPrecision),
    EZ_ENUM_MEMBER_PROPERTY("TexCoordPrecision", ezMeshTexCoordPrecision, m_TexCoordPrecision),
    EZ_ENUM_MEMBER_PROPERTY("VertexColorConversion", ezMeshVertexColorConversion, m_VertexColorConversion),
    EZ_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Radius2", m_fRadius2)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Detail", m_uiDetail)->AddAttributes(new ezDefaultValueAttribute(0), new ezClampValueAttribute(0, 128)),
    EZ_MEMBER_PROPERTY("Detail2", m_uiDetail2)->AddAttributes(new ezDefaultValueAttribute(0), new ezClampValueAttribute(0, 128)),
    EZ_MEMBER_PROPERTY("Cap", m_bCap)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Cap2", m_bCap2)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(360.0f)), new ezClampValueAttribute(ezAngle::MakeFromDegree(0.0f), ezAngle::MakeFromDegree(360.0f))),
    EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new ezContainerAttribute(false, true, true)),
    EZ_MEMBER_PROPERTY("SimplifyMesh", m_bSimplifyMesh),
    EZ_MEMBER_PROPERTY("MeshSimplification", m_uiMeshSimplification)->AddAttributes(new ezDefaultValueAttribute(50), new ezClampValueAttribute(1, 100)),
    EZ_MEMBER_PROPERTY("MaxSimplificationError", m_uiMaxSimplificationError)->AddAttributes(new ezDefaultValueAttribute(5), new ezClampValueAttribute(1, 100)),
    EZ_MEMBER_PROPERTY("AggressiveSimplification", m_bAggressiveSimplification),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class ezMeshAssetPropertiesPatch_1_2 : public ezGraphPatch
{
public:
  ezMeshAssetPropertiesPatch_1_2()
    : ezGraphPatch("ezMeshAssetProperties", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Primitive Type", "PrimitiveType");
    pNode->RenameProperty("Forward Dir", "ForwardDir");
    pNode->RenameProperty("Right Dir", "RightDir");
    pNode->RenameProperty("Up Dir", "UpDir");
    pNode->RenameProperty("Uniform Scaling", "UniformScaling");
    pNode->RenameProperty("Non-Uniform Scaling", "NonUniformScaling");
    pNode->RenameProperty("Mesh File", "MeshFile");
    pNode->RenameProperty("Radius 2", "Radius2");
    pNode->RenameProperty("Detail 2", "Detail2");
    pNode->RenameProperty("Cap 2", "Cap2");
    pNode->RenameProperty("Import Materials", "ImportMaterials");
  }
};

ezMeshAssetPropertiesPatch_1_2 g_MeshAssetPropertiesPatch_1_2;

// clang-format off
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
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezMeshAssetProperties::ezMeshAssetProperties() = default;
ezMeshAssetProperties::~ezMeshAssetProperties() = default;


void ezMeshAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezMeshAssetProperties>())
  {
    const ezInt64 primType = e.m_pObject->GetTypeAccessor().GetValue("PrimitiveType").ConvertTo<ezInt64>();
    const bool bSimplify = e.m_pObject->GetTypeAccessor().GetValue("SimplifyMesh").ConvertTo<bool>();

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
    props["RecalculateNormals"].m_Visibility = ezPropertyUiState::Invisible;
    props["RecalculateTangents"].m_Visibility = ezPropertyUiState::Invisible;
    props["NormalPrecision"].m_Visibility = ezPropertyUiState::Invisible;
    props["TexCoordPrecision"].m_Visibility = ezPropertyUiState::Invisible;
    props["VertexColorConversion"].m_Visibility = ezPropertyUiState::Invisible;

    props["MeshSimplification"].m_Visibility = bSimplify ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MaxSimplificationError"].m_Visibility = bSimplify ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["AggressiveSimplification"].m_Visibility = bSimplify ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

    const ezInt64 importTransform = e.m_pObject->GetTypeAccessor().GetValue("ImportTransform").ConvertTo<ezInt64>();
    const bool bCustomTransform = importTransform == 127;
    props["RightDir"].m_Visibility = bCustomTransform ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["UpDir"].m_Visibility = bCustomTransform ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["FlipForwardDir"].m_Visibility = bCustomTransform ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

    switch (primType)
    {
      case ezMeshPrimitive::File:
        props["MeshFile"].m_Visibility = ezPropertyUiState::Default;
        props["ImportMaterials"].m_Visibility = ezPropertyUiState::Default;
        props["RecalculateNormals"].m_Visibility = ezPropertyUiState::Default;
        props["RecalculateTangents"].m_Visibility = ezPropertyUiState::Default;
        props["NormalPrecision"].m_Visibility = ezPropertyUiState::Default;
        props["TexCoordPrecision"].m_Visibility = ezPropertyUiState::Default;
        props["VertexColorConversion"].m_Visibility = ezPropertyUiState::Default;
        break;

      case ezMeshPrimitive::Box:
        break;

      case ezMeshPrimitive::Rect:
        props["Detail"].m_Visibility = ezPropertyUiState::Default;
        props["Detail2"].m_Visibility = ezPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Rect.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Rect.Detail2";
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

//////////////////////////////////////////////////////////////////////////

class ezMeshAssetPropertiesPatch_2_3 : public ezGraphPatch
{
public:
  ezMeshAssetPropertiesPatch_2_3()
    : ezGraphPatch("ezMeshAssetProperties", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    // convert the "Angle" property from float to ezAngle
    if (auto pProp = pNode->FindProperty("Angle"))
    {
      if (pProp->m_Value.IsA<float>())
      {
        const float valFloat = pProp->m_Value.Get<float>();
        pProp->m_Value = ezAngle::MakeFromDegree(valFloat);
      }
    }
  }
};

ezMeshAssetPropertiesPatch_2_3 g_ezMeshAssetPropertiesPatch_2_3;

//////////////////////////////////////////////////////////////////////////

class ezMeshAssetPropertiesPatch_3_4 : public ezGraphPatch
{
public:
  ezMeshAssetPropertiesPatch_3_4()
    : ezGraphPatch("ezMeshAssetProperties", 4)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("ImportTransform", 127);
  }
};

ezMeshAssetPropertiesPatch_3_4 g_ezMeshAssetPropertiesPatch_3_4;
