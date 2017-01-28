#include <PCH.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoComponent.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoHandle, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Visible", m_bVisible),
    EZ_MEMBER_PROPERTY("Transformation", m_Transformation),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEngineGizmoHandle, 1, ezRTTIDefaultAllocator<ezEngineGizmoHandle>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("HandleType", m_iHandleType),
    EZ_MEMBER_PROPERTY("Color", m_Color),
    EZ_MEMBER_PROPERTY("ConstantSize", m_bConstantSize),
    EZ_MEMBER_PROPERTY("AlwaysOnTop", m_bAlwaysOnTop),
    EZ_MEMBER_PROPERTY("Visualizer", m_bVisualizer),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezGizmoHandle::ezGizmoHandle()
{
  m_bVisible = false;
  m_Transformation.SetZero();
  m_pParentGizmo = nullptr;
}

void ezGizmoHandle::SetVisible(bool bVisible)
{
  if (bVisible != m_bVisible)
  {
    m_bVisible = bVisible;
    SetModified(true);
  }
}

void ezGizmoHandle::SetTransformation(const ezMat4& m)
{
  if (m_Transformation != m)
  {
    m_Transformation = m;
    if (m_bVisible)
      SetModified(true);
  }
}


static ezMeshBufferResourceHandle CreateMeshBufferResource(const ezGeometry& geom, const char* szResourceName, const char* szDescription, ezGALPrimitiveTopology::Enum topology)
{
  ezMeshBufferResourceDescriptor desc;
  desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  desc.AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);
  desc.AllocateStreamsFromGeometry(geom, topology);

  return ezResourceManager::CreateResource<ezMeshBufferResource>(szResourceName, desc, szDescription);
}

static ezMeshBufferResourceHandle CreateMeshBufferArrow()
{
  const char* szResourceName = "{B9DC6776-38D8-4C1F-994F-225E69E71283}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fThickness = 0.04f;
  const float fLength = 2.0f;

  ezMat4 m;
  m.SetRotationMatrixY(ezAngle::Degree(90));

  ezGeometry geom;
  geom.AddCylinder(fThickness, fThickness, fLength, false, true, 16, ezColor::Red, m);

  m.SetTranslationVector(ezVec3(fLength * 0.5f, 0, 0));
  geom.AddCone(fThickness * 3.0f, fThickness * 6.0f, true, 16, ezColor::Red, m);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Arrow", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferPiston()
{
  const char* szResourceName = "{E2B59B8F-8F61-48C0-AE37-CF31107BA2CE}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fThickness = 0.04f;
  const float fLength = 2.0f;

  ezMat4 m;
  m.SetRotationMatrixY(ezAngle::Degree(90));

  ezGeometry geom;
  geom.AddCylinder(fThickness, fThickness, fLength, false, true, 16, ezColor::Red, m);

  m.SetTranslationVector(ezVec3(fLength * 0.5f, 0, 0));
  geom.AddBox(ezVec3(fThickness * 5.0f), ezColor::Red, m);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Piston", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferHalfPiston()
{
  const char* szResourceName = "{BA17D025-B280-4940-8DFD-5486B0E4B41B}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fThickness = 0.04f;
  const float fLength = 1.0f;

  ezMat4 m;
  m.SetRotationMatrixY(ezAngle::Degree(90));
  m.SetTranslationVector(ezVec3(fLength * 0.5f, 0, 0));

  ezGeometry geom;
  geom.AddCylinder(fThickness, fThickness, fLength, false, true, 16, ezColor::Red, m);

  m.SetTranslationVector(ezVec3(fLength, 0, 0));
  geom.AddBox(ezVec3(fThickness * 5.0f), ezColor::Red, m);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_HalfPiston", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferRect()
{
  const char* szResourceName = "{75597E89-CDEE-4C90-A377-9441F64B9DB2}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fLength = 2.0f / 3.0f;

  ezMat4 m;
  m.SetIdentity();

  ezGeometry geom;
  geom.AddRectXY(ezVec2(fLength), ezColor::White, m);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Rect", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferRing()
{
  const char* szResourceName = "{EA8677E3-F623-4FD8-BFAB-349CE1BEB3CA}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fInnerRadius = 1.3f;
  const float fOuterRadius = fInnerRadius + 0.1f;

  ezMat4 m;
  m.SetIdentity();

  ezGeometry geom;
  geom.AddTorus(fInnerRadius, fOuterRadius, 32, 8, ezColor::White);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Ring", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferBox()
{
  const char* szResourceName = "{F14D4CD3-8F21-442B-B07F-3567DBD58A3F}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezMat4 m;
  m.SetIdentity();

  ezGeometry geom;
  geom.AddBox(ezVec3(1.0f), ezColor::White, m);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Box", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferLineBox()
{
  const char* szResourceName = "{55DF000E-EE88-4BDC-8A7B-FA496941064E}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezMat4 m;
  m.SetIdentity();

  ezGeometry geom;
  geom.AddLineBox(ezVec3(1.0f), ezColor::White, m);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_LineBox", ezGALPrimitiveTopology::Lines);
}

static ezMeshBufferResourceHandle CreateMeshBufferSphere()
{
  const char* szResourceName = "{A88779B0-4728-4411-A9D7-532AFE6F4704}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezGeometry geom;
  geom.AddGeodesicSphere(1.0f, 2, ezColor::White);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Sphere", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferCylinderZ()
{
  const char* szResourceName = "{3BBE2251-0DE4-4B71-979E-A407D8F5CB59}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezGeometry geom;
  geom.AddCylinder(1.0f, 1.0f, 1.0f, false, false, 16, ezColor::White);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_CylinderZ", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferHalfSphereZ()
{
  const char* szResourceName = "{05BDED8B-96C1-4F2E-8F1B-5C07B3C28D22}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezGeometry geom;
  geom.AddHalfSphere(1.0f, 16, 8, false, ezColor::White);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_HalfSphereZ", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferBoxFaces()
{
  const char* szResourceName = "{BD925A8E-480D-41A6-8F62-0AC5F72DA4F6}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezMat4 t;

  ezGeometry geom;
  t.SetTranslationMatrix(ezVec3(0, 0, 0.5f));
  geom.AddRectXY(ezVec2(0.5f), ezColor::White, t);

  t.SetRotationMatrixY(ezAngle::Degree(180.0));
  t.SetTranslationVector(ezVec3(0, 0, -0.5f));
  geom.AddRectXY(ezVec2(0.5f), ezColor::White, t);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_BoxFaces", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferBoxEdges()
{
  const char* szResourceName = "{FE700F28-514E-4193-A0F6-4351E0BAC222}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezMat4 t, rot;

  ezGeometry geom;

  for (ezUInt32 i = 0; i < 4; ++i)
  {
    rot.SetRotationMatrixY(ezAngle::Degree(90.0f * i));

    t.SetTranslationMatrix(ezVec3(0.5f - 0.125f, 0, 0.5f));
    geom.AddRectXY(ezVec2(0.25f, 0.5f), ezColor::White, rot * t);

    t.SetTranslationMatrix(ezVec3(-0.5f + 0.125f, 0, 0.5f));
    geom.AddRectXY(ezVec2(0.25f, 0.5f), ezColor::White, rot * t);
  }

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_BoxEdges", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferBoxCorners()
{
  const char* szResourceName = "{FBDB6A82-D4B0-447F-815B-228D340451CB}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezMat4 t, rot[6];
  rot[0].SetIdentity();
  rot[1].SetRotationMatrixX(ezAngle::Degree(90));
  rot[2].SetRotationMatrixX(ezAngle::Degree(180));
  rot[3].SetRotationMatrixX(ezAngle::Degree(270));
  rot[4].SetRotationMatrixY(ezAngle::Degree(90));
  rot[5].SetRotationMatrixY(ezAngle::Degree(-90));

  ezGeometry geom;

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    t.SetTranslationMatrix(ezVec3(0.5f - 0.125f, 0.5f - 0.125f, 0.5f));
    geom.AddRectXY(ezVec2(0.25f, 0.25f), ezColor::White, rot[i] * t);

    t.SetTranslationMatrix(ezVec3(0.5f - 0.125f, -0.5f + 0.125f, 0.5f));
    geom.AddRectXY(ezVec2(0.25f, 0.25f), ezColor::White, rot[i] * t);

    t.SetTranslationMatrix(ezVec3(-0.5f + 0.125f, 0.5f - 0.125f, 0.5f));
    geom.AddRectXY(ezVec2(0.25f, 0.25f), ezColor::White, rot[i] * t);

    t.SetTranslationMatrix(ezVec3(-0.5f + 0.125f, -0.5f + 0.125f, 0.5f));
    geom.AddRectXY(ezVec2(0.25f, 0.25f), ezColor::White, rot[i] * t);
  }

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_BoxCorners", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferCone()
{
  const char* szResourceName = "{BED97C9E-4E7A-486C-9372-1FB1A5FAE786}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezMat4 t;
  t.SetRotationMatrixY(ezAngle::Degree(270.0f));
  t.SetTranslationVector(ezVec3(1.0f, 0, 0));

  ezGeometry geom;
  geom.AddCone(1.0f, 1.0f, false, 16, ezColor::White, t);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Cone", ezGALPrimitiveTopology::Triangles);
}

static ezMeshResourceHandle CreateMeshResource(const char* szMeshResourceName, ezMeshBufferResourceHandle hMeshBuffer, const char* szMaterial)
{
  const ezStringBuilder sIdentifier(szMeshResourceName, "@", szMaterial);

  ezMeshResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sIdentifier);

  if (hMesh.IsValid())
    return hMesh;

  ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer);

  ezMeshResourceDescriptor md;
  md.UseExistingMeshBuffer(hMeshBuffer);
  md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
  md.SetMaterial(0, szMaterial);
  md.CalculateBounds();

  return ezResourceManager::CreateResource<ezMeshResource>(sIdentifier, md, pMeshBuffer->GetResourceDescription());
}

ezEngineGizmoHandle::ezEngineGizmoHandle()
{
  m_iHandleType = -1;
  m_pGizmoComponent = nullptr;
  m_bConstantSize = true;
  m_bAlwaysOnTop = false;
  m_bVisualizer = false;
  m_Color = ezColor::CornflowerBlue; /* The Original! */
}

ezEngineGizmoHandle::~ezEngineGizmoHandle()
{
  if (m_hGameObject.IsInvalidated())
    return;

  m_pWorld->DeleteObjectDelayed(m_hGameObject);
}

void ezEngineGizmoHandle::Configure(ezGizmo* pParentGizmo, ezEngineGizmoHandleType type, const ezColor& col, bool bConstantSize, bool bAlwaysOnTop, bool bVisualizer)
{
  SetParentGizmo(pParentGizmo);

  m_bConstantSize = bConstantSize;
  m_bAlwaysOnTop = bAlwaysOnTop;
  m_bVisualizer = bVisualizer;
  m_iHandleType = (int)type;
  m_Color = col;
}

bool ezEngineGizmoHandle::SetupForEngine(ezWorld* pWorld, ezUInt32 uiNextComponentPickingID)
{
  m_pWorld = pWorld;

  if (!m_hGameObject.IsInvalidated())
    return false;

  ezMeshBufferResourceHandle hMeshBuffer;
  const char* szMeshGuid = "";

  switch (m_iHandleType)
  {
  case ezEngineGizmoHandleType::Arrow:
    {
      hMeshBuffer = CreateMeshBufferArrow();
      szMeshGuid = "{9D02CF27-7A15-44EA-A372-C417AF2A8E9B}";
    }
    break;
  case ezEngineGizmoHandleType::Rect:
    {
      hMeshBuffer = CreateMeshBufferRect();
      szMeshGuid = "{3DF4DDDA-F598-4A37-9691-D4C3677905A8}";
    }
    break;
  case ezEngineGizmoHandleType::Ring:
    {
      hMeshBuffer = CreateMeshBufferRing();
      szMeshGuid = "{629AD0C6-C81B-4850-A5BC-41494DC0BF95}";
    }
    break;
  case ezEngineGizmoHandleType::Box:
    {
      hMeshBuffer = CreateMeshBufferBox();
      szMeshGuid = "{13A59253-4A98-4638-8B94-5AA370E929A7}";
    }
    break;
  case ezEngineGizmoHandleType::Piston:
    {
      hMeshBuffer = CreateMeshBufferPiston();
      szMeshGuid = "{44A4FE37-6AE3-44C1-897D-E8B95AE53EF6}";
    }
    break;
  case ezEngineGizmoHandleType::HalfPiston:
    {
      hMeshBuffer = CreateMeshBufferHalfPiston();
      szMeshGuid = "{64A45DD0-D7F9-4D1D-9F68-782FA3274200}";
    }
    break;
  case ezEngineGizmoHandleType::Sphere:
    {
      hMeshBuffer = CreateMeshBufferSphere();
      szMeshGuid = "{FC322E80-5EB0-452F-9D8E-9E65FCFDA652}";
    }
    break;
  case ezEngineGizmoHandleType::CylinderZ:
    {
      hMeshBuffer = CreateMeshBufferCylinderZ();
      szMeshGuid = "{893384EA-2F43-4265-AF75-662E2C81C167}";
    }
    break;
  case ezEngineGizmoHandleType::HalfSphereZ:
    {
      hMeshBuffer = CreateMeshBufferHalfSphereZ();
      szMeshGuid = "{0FC9B680-7B6B-40B6-97BD-CBFFA47F0EFF}";
    }
    break;
  case ezEngineGizmoHandleType::BoxCorners:
    {
      hMeshBuffer = CreateMeshBufferBoxCorners();
      szMeshGuid = "{89CCC389-11D5-43F4-9C18-C634EE3154B9}";
    }
    break;
  case ezEngineGizmoHandleType::BoxEdges:
    {
      hMeshBuffer = CreateMeshBufferBoxEdges();
      szMeshGuid = "{21508253-2E74-44CE-9399-523214BE7C3D}";
    }
    break;
  case ezEngineGizmoHandleType::BoxFaces:
    {
      hMeshBuffer = CreateMeshBufferBoxFaces();
      szMeshGuid = "{FD1A3C29-F8F0-42B0-BBB0-D0A2B28A65A0}";
    }
    break;
  case ezEngineGizmoHandleType::LineBox:
    {
      hMeshBuffer = CreateMeshBufferLineBox();
      szMeshGuid = "{4B136D72-BF43-4C4B-96D7-51C5028A7006}";
    }
    break;
  case ezEngineGizmoHandleType::Cone:
    {
      hMeshBuffer = CreateMeshBufferCone();
      szMeshGuid = "{9A48962D-127A-445C-899A-A054D6AD8A9A}";
    }
    break;
  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  ezGameObjectDesc god;
  god.m_LocalPosition = m_Transformation.GetTranslationVector();
  god.m_LocalRotation.SetFromMat3(m_Transformation.GetRotationalPart());
  god.m_LocalScaling = m_Transformation.GetScalingFactors();

  ezGameObject* pObject;
  m_hGameObject = pWorld->CreateObject(god, pObject);

  {
    const ezTag* tagNoOrtho = ezTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    pObject->GetTags().Set(*tagNoOrtho);
  }

  {
    const ezTag* tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

    pObject->GetTags().Set(*tagEditor);
  }

  ezGizmoComponent::CreateComponent(pWorld, m_pGizmoComponent);


  ezMeshResourceHandle hMesh;

  if (m_bVisualizer)
  {
    hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Materials/Editor/Visualizer.ezMaterial");
  }
  else if (m_bConstantSize)
  {
    hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Materials/Editor/GizmoHandleConstantSize.ezMaterial");
  }
  else
  {
    hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Materials/Editor/GizmoHandle.ezMaterial");
  }

  m_pGizmoComponent->SetRenderDataCategory(m_bVisualizer ? ezDefaultRenderDataCategories::SimpleOpaque : ezDefaultRenderDataCategories::SimpleForeground);
  m_pGizmoComponent->m_GizmoColor = m_Color;
  m_pGizmoComponent->m_bUseDepthPrepass = !m_bVisualizer;
  m_pGizmoComponent->SetMesh(hMesh);

  m_pGizmoComponent->SetUniqueID(uiNextComponentPickingID);

  pObject->AttachComponent(m_pGizmoComponent);

  return true;
}

void ezEngineGizmoHandle::UpdateForEngine(ezWorld* pWorld)
{
  if (m_hGameObject.IsInvalidated())
    return;

  ezGameObject* pObject;
  if (!pWorld->TryGetObject(m_hGameObject, pObject))
    return;

  ezTransform t;
  t.m_vPosition = m_Transformation.GetTranslationVector();
  t.m_Rotation = m_Transformation.GetRotationalPart();

  ezQuat qRot;
  ezVec3 vTrans, vScale;
  t.Decompose(vTrans, qRot, vScale);

  pObject->SetLocalPosition(vTrans);
  pObject->SetLocalRotation(qRot);
  pObject->SetLocalScaling(vScale);

  m_pGizmoComponent->m_GizmoColor = m_Color;
  m_pGizmoComponent->m_bUseDepthPrepass = !m_bVisualizer;
  m_pGizmoComponent->SetActive(m_bVisible);
}

void ezEngineGizmoHandle::SetColor(const ezColor& col)
{
  m_Color = col;
  SetModified();
}



