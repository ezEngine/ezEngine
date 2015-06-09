#include <PCH.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoHandleBase, ezEditorEngineSyncObject, 1, ezRTTINoAllocator);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Visible", m_bVisible),
EZ_MEMBER_PROPERTY("Transformation", m_Transformation),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoHandle, ezGizmoHandleBase, 1, ezRTTIDefaultAllocator<ezGizmoHandle>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("HandleType", m_iHandleType),
EZ_MEMBER_PROPERTY("Color", m_Color),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

static ezMeshBufferResourceHandle CreateMeshBufferResource(const ezGeometry& geom, const char* szResourceName)
{
  ezDynamicArray<ezUInt16> Indices;
  Indices.Reserve(geom.GetPolygons().GetCount() * 6);

  for (ezUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
  {
    for (ezUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
    {
      Indices.PushBack(geom.GetPolygons()[p].m_Vertices[0]);
      Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 1]);
      Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 2]);
    }
  }

  ezMeshBufferResourceDescriptor desc;
  desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  desc.AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);

  desc.AllocateStreams(geom.GetVertices().GetCount(), Indices.GetCount() / 3);

  for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
  {
    desc.SetVertexData<ezVec3>(0, v, geom.GetVertices()[v].m_vPosition);
    desc.SetVertexData<ezColorGammaUB>(1, v, geom.GetVertices()[v].m_Color);
  }

  for (ezUInt32 t = 0; t < Indices.GetCount(); t += 3)
  {
    desc.SetTriangleIndices(t / 3, Indices[t], Indices[t + 1], Indices[t + 2]);
  }

  return ezResourceManager::CreateResource<ezMeshBufferResource>(szResourceName, desc);
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
  m.SetIdentity();

  ezGeometry geom;
  geom.AddCylinder(fThickness, fThickness, fLength, false, true, 16, ezColor::Red, m);

  m.SetTranslationVector(ezVec3(0, fLength * 0.5f, 0));
  geom.AddCone(fThickness * 3.0f, fThickness * 6.0f, true, 16, ezColor::Red, m);

  return CreateMeshBufferResource(geom, szResourceName);
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

  return CreateMeshBufferResource(geom, szResourceName);
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
  geom.AddTorus(fInnerRadius, fOuterRadius, 32, 8,ezColor::White);

  return CreateMeshBufferResource(geom, szResourceName);
}

static ezMeshResourceHandle CreateMeshResource(const char* szMeshResourceName, ezMeshBufferResourceHandle hMeshBuffer, const char* szMaterial)
{
  ezMeshResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer);

  ezMeshResourceDescriptor md;
  md.UseExistingMeshBuffer(hMeshBuffer);
  md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
  md.SetMaterial(0, szMaterial);

  return ezResourceManager::CreateResource<ezMeshResource>(szMeshResourceName, md);
}

ezGizmoHandle::ezGizmoHandle()
{
  m_bVisible = false;
  m_Transformation.SetIdentity();
  m_Color = ezColor::CornflowerBlue; /* The Original! */
  m_pMeshComponent = nullptr;
}

void ezGizmoHandle::Configure(ezGizmoBase* pParentGizmo, ezGizmoHandleType type, const ezColor& col)
{
  SetParentGizmo(pParentGizmo);

  m_iHandleType = (int)type;
  m_Color = col;
}

bool ezGizmoHandle::SetupForEngine(ezWorld* pWorld, ezUInt32 uiNextComponentPickingID)
{
  if (!m_hGameObject.IsInvalidated())
    return false;

  ezMeshBufferResourceHandle hMeshBuffer;
  const char* szMeshGuid = "";

  switch (m_iHandleType)
  {
  case ezGizmoHandleType::Arrow:
    {
      hMeshBuffer = CreateMeshBufferArrow();
      szMeshGuid = "{9D02CF27-7A15-44EA-A372-C417AF2A8E9B}";
    }
    break;
  case ezGizmoHandleType::Rect:
    {
      hMeshBuffer = CreateMeshBufferRect();
      szMeshGuid = "{3DF4DDDA-F598-4A37-9691-D4C3677905A8}";
    }
    break;
  case ezGizmoHandleType::Ring:
    {
      hMeshBuffer = CreateMeshBufferRing();
      szMeshGuid = "{629AD0C6-C81B-4850-A5BC-41494DC0BF95}";
    }
    break;
  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  ezMeshResourceHandle hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Materials/Editor/GizmoHandle.ezMaterial");

  ezGameObjectDesc god;
  god.m_LocalPosition = m_Transformation.GetTranslationVector();
  god.m_LocalRotation.SetFromMat3(m_Transformation.GetRotationalPart());
  god.m_LocalScaling = m_Transformation.GetScalingFactors();

  ezGameObject* pObject;
  m_hGameObject = pWorld->CreateObject(god, pObject);

  ezMeshComponentManager* pMeshCompMan = pWorld->GetComponentManager<ezMeshComponentManager>();
  pMeshCompMan->CreateComponent(m_pMeshComponent);

  m_pMeshComponent->m_MeshColor = m_Color;
  m_pMeshComponent->SetMesh(hMesh);
  m_pMeshComponent->SetRenderPass(ezDefaultPassTypes::Foreground);
  m_pMeshComponent->m_uiEditorPickingID = uiNextComponentPickingID;

  pObject->AddComponent(m_pMeshComponent);

  return true;
}

void ezGizmoHandle::UpdateForEngine(ezWorld* pWorld)
{
  if (m_hGameObject.IsInvalidated())
    return;

  ezGameObject* pObject;
  if (!pWorld->TryGetObject(m_hGameObject, pObject))
    return;

  ezQuat qRot;
  qRot.SetFromMat3(m_Transformation.GetRotationalPart());

  pObject->SetLocalPosition(m_Transformation.GetTranslationVector());
  pObject->SetLocalRotation(qRot);
  pObject->SetLocalScaling(m_Transformation.GetScalingFactors());

  m_pMeshComponent->SetActive(m_bVisible);
}