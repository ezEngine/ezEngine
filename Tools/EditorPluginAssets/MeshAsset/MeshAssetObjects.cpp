#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetProperties, 1, ezRTTIDefaultAllocator<ezMeshAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Primitive Type", ezMeshPrimitive, m_PrimitiveType),
    EZ_ENUM_MEMBER_PROPERTY("Forward Dir", ezBasisAxis, m_ForwardDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveX)),
    EZ_ENUM_MEMBER_PROPERTY("Right Dir", ezBasisAxis, m_RightDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),
    EZ_ENUM_MEMBER_PROPERTY("Up Dir", ezBasisAxis, m_UpDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveZ)),
    EZ_MEMBER_PROPERTY("Uniform Scaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Non-Uniform Scaling", m_vNonUniformScaling)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f))),
    EZ_MEMBER_PROPERTY("Mesh File", m_sMeshFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", "*.obj;*.fbx;*.ply;*.pbrt")), // todo. need to get this list of extensions automatically.
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Radius 2", m_fRadius2)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Detail", m_uiDetail)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 128)),
    EZ_MEMBER_PROPERTY("Detail 2", m_uiDetail2)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 128)),
    EZ_MEMBER_PROPERTY("Cap", m_bCap)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Cap 2", m_bCap2)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Angle", m_fAngle)->AddAttributes(new ezDefaultValueAttribute(360.0f), new ezClampValueAttribute(0.0f, 360.0f)),
    EZ_MEMBER_PROPERTY("Import Materials", m_bImportMaterials)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new ezContainerAttribute(false, false, true)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

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
    ezInt64 primType = e.m_pObject->GetTypeAccessor().GetValue("Primitive Type").ConvertTo<ezInt64>();

    auto& props = *e.m_pPropertyStates;

    props["Mesh File"].m_Visibility = ezPropertyUiState::Invisible;
    props["Radius"].m_Visibility = ezPropertyUiState::Invisible;
    props["Radius 2"].m_Visibility = ezPropertyUiState::Invisible;
    props["Height"].m_Visibility = ezPropertyUiState::Invisible;
    props["Detail"].m_Visibility = ezPropertyUiState::Invisible;
    props["Detail 2"].m_Visibility = ezPropertyUiState::Invisible;
    props["Cap"].m_Visibility = ezPropertyUiState::Invisible;
    props["Cap 2"].m_Visibility = ezPropertyUiState::Invisible;
    props["Angle"].m_Visibility = ezPropertyUiState::Invisible;
    props["Import Materials"].m_Visibility = ezPropertyUiState::Invisible;

    switch (primType)
    {
    case ezMeshPrimitive::File:
      props["Mesh File"].m_Visibility = ezPropertyUiState::Default;
      props["Import Materials"].m_Visibility = ezPropertyUiState::Default;
      break;

    case ezMeshPrimitive::Box:
    case ezMeshPrimitive::Rect:
      break;

    case ezMeshPrimitive::Capsule:
      props["Radius"].m_Visibility = ezPropertyUiState::Default;
      props["Height"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Detail 2"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.Sphere.Detail1";
      props["Detail 2"].m_sNewLabelText = "Prim.Sphere.Detail2";
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
      props["Radius 2"].m_Visibility = ezPropertyUiState::Default;
      props["Height"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Cap"].m_Visibility = ezPropertyUiState::Default;
      props["Cap 2"].m_Visibility = ezPropertyUiState::Default;
      props["Angle"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.Cylinder.Detail";
      props["Radius"].m_sNewLabelText = "Prim.Cylinder.Radius1";
      props["Radius 2"].m_sNewLabelText = "Prim.Cylinder.Radius2";
      props["Angle"].m_sNewLabelText = "Prim.Cylinder.Angle";
      props["Cap"].m_sNewLabelText = "Prim.Cylinder.Cap1";
      props["Cap 2"].m_sNewLabelText = "Prim.Cylinder.Cap2";
      break;

    case ezMeshPrimitive::GeodesicSphere:
      props["Radius"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.GeoSphere.Detail";
      break;

    case ezMeshPrimitive::HalfSphere:
      props["Radius"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Detail 2"].m_Visibility = ezPropertyUiState::Default;
      props["Cap"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.Sphere.Detail1";
      props["Detail 2"].m_sNewLabelText = "Prim.Sphere.Detail2";
      break;

    case ezMeshPrimitive::Pyramid:
      props["Cap"].m_Visibility = ezPropertyUiState::Default;
      break;

    case ezMeshPrimitive::Sphere:
      props["Radius"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Detail 2"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.Sphere.Detail1";
      props["Detail 2"].m_sNewLabelText = "Prim.Sphere.Detail2";
      break;

    case ezMeshPrimitive::Torus:
      props["Radius"].m_Visibility = ezPropertyUiState::Default;
      props["Radius 2"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Detail 2"].m_Visibility = ezPropertyUiState::Default;

      props["Detail"].m_sNewLabelText = "Prim.Torus.Detail1";
      props["Detail 2"].m_sNewLabelText = "Prim.Torus.Detail2";
      props["Radius"].m_sNewLabelText = "Prim.Torus.Radius1";
      props["Radius 2"].m_sNewLabelText = "Prim.Torus.Radius2";
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


