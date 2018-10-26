#include <PCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoHandle, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Visible", m_bVisible),
    EZ_MEMBER_PROPERTY("Transformation", m_Transformation),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEngineGizmoHandle, 1, ezRTTIDefaultAllocator<ezEngineGizmoHandle>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("HandleType", m_iHandleType),
    EZ_MEMBER_PROPERTY("Color", m_Color),
    EZ_MEMBER_PROPERTY("ConstantSize", m_bConstantSize),
    EZ_MEMBER_PROPERTY("AlwaysOnTop", m_bAlwaysOnTop),
    EZ_MEMBER_PROPERTY("Visualizer", m_bVisualizer),
    EZ_MEMBER_PROPERTY("Ortho", m_bShowInOrtho),
    EZ_MEMBER_PROPERTY("Pickable", m_bIsPickable),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezGizmoHandle::ezGizmoHandle()
{
  m_bVisible = false;
  m_Transformation.SetIdentity();
  m_Transformation.m_vScale.SetZero(); // make sure it is different from anything valid
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

void ezGizmoHandle::SetTransformation(const ezTransform& m)
{
  if (m_Transformation != m)
  {
    m_Transformation = m;
    if (m_bVisible)
      SetModified(true);
  }
}

void ezGizmoHandle::SetTransformation(const ezMat4& m)
{
  ezTransform t;
  t.SetFromMat4(m);
  SetTransformation(t);
}

static ezMeshBufferResourceHandle CreateMeshBufferResource(const ezGeometry& geom, const char* szResourceName, const char* szDescription,
                                                           ezGALPrimitiveTopology::Enum topology)
{
  ezMeshBufferResourceDescriptor desc;
  desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  desc.AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);
  desc.AllocateStreamsFromGeometry(geom, topology);
  desc.ComputeBounds();

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
  geom.AddCylinder(fThickness, fThickness, fLength * 0.5f, fLength * 0.5f, false, true, 16, ezColor::Red, m);

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
  geom.AddCylinder(fThickness, fThickness, fLength * 0.5f, fLength * 0.5f, false, true, 16, ezColor::Red, m);

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
  geom.AddCylinder(fThickness, fThickness, fLength * 0.5f, fLength * 0.5f, false, true, 16, ezColor::Red, m);

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

  // weird size because of translate gizmo, should be fixed through scaling there instead
  const float fLength = 2.0f / 3.0f;

  ezMat4 m;
  m.SetIdentity();

  ezGeometry geom;
  geom.AddRectXY(ezVec2(fLength), ezColor::White, m);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Rect", ezGALPrimitiveTopology::Triangles);
}

static ezMeshBufferResourceHandle CreateMeshBufferLineRect()
{
  const char* szResourceName = "{A1EA52B0-DA73-4176-B50D-3470DDB053F8}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezMat4 m;
  m.SetIdentity();

  ezGeometry geom;

  const ezVec2 halfSize(1.0f);

  geom.AddVertex(ezVec3(-halfSize.x, -halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(0, 1), ezColor::White, 0, m);
  geom.AddVertex(ezVec3(halfSize.x, -halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(0, 0), ezColor::White, 0, m);
  geom.AddVertex(ezVec3(halfSize.x, halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(1, 0), ezColor::White, 0, m);
  geom.AddVertex(ezVec3(-halfSize.x, halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(1, 1), ezColor::White, 0, m);

  geom.AddLine(0, 1);
  geom.AddLine(1, 2);
  geom.AddLine(2, 3);
  geom.AddLine(3, 0);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_LineRect", ezGALPrimitiveTopology::Lines);
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
  geom.AddCylinder(1.0f, 1.0f, 0.5f, 0.5f, false, false, 16, ezColor::White);

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

static ezMeshBufferResourceHandle CreateMeshBufferFrustum()
{
  const char* szResourceName = "{61A7BE38-797D-4BFC-AED6-33CE4F4C6FF6}";

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezMat4 m;
  m.SetIdentity();

  ezGeometry geom;

  geom.AddVertex(ezVec3(0, 0, 0), ezVec3(0, 0, 1), ezVec2(0), ezColor::White, 0, m);

  geom.AddVertex(ezVec3(1.0f, -1.0f, 1.0f), ezVec3(0, 0, 1), ezVec2(0), ezColor::White, 0, m);
  geom.AddVertex(ezVec3(1.0f, 1.0f, 1.0f), ezVec3(0, 0, 1), ezVec2(0), ezColor::White, 0, m);
  geom.AddVertex(ezVec3(1.0f, -1.0f, -1.0f), ezVec3(0, 0, 1), ezVec2(0), ezColor::White, 0, m);
  geom.AddVertex(ezVec3(1.0f, 1.0f, -1.0f), ezVec3(0, 0, 1), ezVec2(0), ezColor::White, 0, m);

  geom.AddLine(0, 1);
  geom.AddLine(0, 2);
  geom.AddLine(0, 3);
  geom.AddLine(0, 4);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Frustum", ezGALPrimitiveTopology::Lines);
}

static ezMeshResourceHandle CreateMeshResource(const char* szMeshResourceName, ezMeshBufferResourceHandle hMeshBuffer,
                                               const char* szMaterial)
{
  const ezStringBuilder sIdentifier(szMeshResourceName, "-with-", szMaterial);

  ezMeshResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sIdentifier);

  if (hMesh.IsValid())
    return hMesh;

  ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer);

  ezMeshResourceDescriptor md;
  md.UseExistingMeshBuffer(hMeshBuffer);
  md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
  md.SetMaterial(0, szMaterial);
  md.ComputeBounds();

  return ezResourceManager::CreateResource<ezMeshResource>(sIdentifier, md, pMeshBuffer->GetResourceDescription());
}

ezEngineGizmoHandle::ezEngineGizmoHandle()
{
  m_iHandleType = -1;
  m_pGizmoComponent = nullptr;
  m_bConstantSize = true;
  m_bAlwaysOnTop = false;
  m_bVisualizer = false;
  m_bShowInOrtho = false;
  m_Color = ezColor::CornflowerBlue; /* The Original! */
  m_pWorld = nullptr;
}

ezEngineGizmoHandle::~ezEngineGizmoHandle()
{
  if (m_hGameObject.IsInvalidated())
    return;

  m_pWorld->DeleteObjectDelayed(m_hGameObject);
}

void ezEngineGizmoHandle::Configure(ezGizmo* pParentGizmo, ezEngineGizmoHandleType type, const ezColor& col, bool bConstantSize,
                                    bool bAlwaysOnTop, bool bVisualizer, bool bShowInOrtho, bool bIsPickable)
{
  SetParentGizmo(pParentGizmo);

  m_bConstantSize = bConstantSize;
  m_bAlwaysOnTop = bAlwaysOnTop;
  m_bVisualizer = bVisualizer;
  m_iHandleType = (int)type;
  m_Color = col;
  m_bShowInOrtho = bShowInOrtho;
  m_bIsPickable = bIsPickable;
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
    case ezEngineGizmoHandleType::LineRect:
    {
      hMeshBuffer = CreateMeshBufferLineRect();
      szMeshGuid = "{96129543-897C-4DEE-922D-931BC91C5725}";
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
    case ezEngineGizmoHandleType::Frustum:
    {
      szMeshGuid = "{22EC5D48-E8BE-410B-8EAD-51B7775BA058}";
      hMeshBuffer = CreateMeshBufferFrustum();
    }
    break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  ezStringBuilder sName;
  sName.Format("Gizmo{0}", m_iHandleType);

  ezGameObjectDesc god;
  god.m_LocalPosition = m_Transformation.m_vPosition;
  god.m_LocalRotation = m_Transformation.m_qRotation;
  god.m_LocalScaling = m_Transformation.m_vScale;
  god.m_sName.Assign(sName.GetData());
  god.m_bDynamic = true;

  ezGameObject* pObject;
  m_hGameObject = pWorld->CreateObject(god, pObject);

  if (!m_bShowInOrtho)
  {
    const ezTag& tagNoOrtho = ezTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    pObject->GetTags().Set(tagNoOrtho);
  }

  {
    const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

    pObject->GetTags().Set(tagEditor);
  }

  ezGizmoComponent::CreateComponent(pObject, m_pGizmoComponent);


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

  m_pGizmoComponent->SetRenderDataCategory(m_bVisualizer ? ezDefaultRenderDataCategories::SimpleOpaque
                                                         : ezDefaultRenderDataCategories::SimpleForeground);
  m_pGizmoComponent->m_GizmoColor = m_Color;
  m_pGizmoComponent->m_bUseDepthPrepass = !m_bVisualizer;
  m_pGizmoComponent->m_bIsPickable = m_bIsPickable;
  m_pGizmoComponent->SetMesh(hMesh);

  m_pGizmoComponent->SetUniqueID(uiNextComponentPickingID);

  return true;
}

void ezEngineGizmoHandle::UpdateForEngine(ezWorld* pWorld)
{
  if (m_hGameObject.IsInvalidated())
    return;

  ezGameObject* pObject;
  if (!pWorld->TryGetObject(m_hGameObject, pObject))
    return;

  pObject->SetLocalPosition(m_Transformation.m_vPosition);
  pObject->SetLocalRotation(m_Transformation.m_qRotation);
  pObject->SetLocalScaling(m_Transformation.m_vScale);

  m_pGizmoComponent->m_GizmoColor = m_Color;
  m_pGizmoComponent->m_bUseDepthPrepass = !m_bVisualizer;
  m_pGizmoComponent->SetActive(m_bVisible);
}

void ezEngineGizmoHandle::SetColor(const ezColor& col)
{
  m_Color = col;
  SetModified();
}
