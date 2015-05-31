#include <PCH.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorGizmoHandle, ezEditorEngineSyncObject, 1, ezRTTIDefaultAllocator<ezEditorGizmoHandle>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Visible", m_bVisible),
EZ_MEMBER_PROPERTY("Transformation", m_Transformation),
EZ_MEMBER_PROPERTY("HandleType", m_iHandleType),
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

  const float fThickness = 0.05f;
  const float fLength = 1.0f;

  ezMat4 m;
  m.SetIdentity();

  ezGeometry geom;
  geom.AddCylinder(fThickness, fThickness, fLength, false, true, 16, ezColor::Red, m);

  m.SetTranslationVector(ezVec3(0, fLength * 0.5f, 0));
  geom.AddCone(fThickness * 3.0f, fThickness * 6.0f, true, 16, ezColor::Red, m);

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

void ezEditorGizmoHandle::SetupForEngine(ezWorld* pWorld)
{
  if (!m_hGameObject.IsInvalidated())
    return;

  ezMeshBufferResourceHandle hMeshBuffer = CreateMeshBufferArrow();
  ezMeshResourceHandle hMesh = CreateMeshResource("{9D02CF27-7A15-44EA-A372-C417AF2A8E9B}", hMeshBuffer, "Materials/Editor/GizmoHandle.ezMaterial");

  ezGameObjectDesc god;
  god.m_LocalPosition = m_Transformation.GetTranslationVector();
  god.m_LocalRotation.SetFromMat3(m_Transformation.GetRotationalPart());
  god.m_LocalScaling = m_Transformation.GetScalingFactors();

  ezGameObject* pObject;
  m_hGameObject = pWorld->CreateObject(god, pObject);

  ezMeshComponentManager* pMeshCompMan = pWorld->GetComponentManager<ezMeshComponentManager>();

  ezMeshComponent* pMeshComponent;
  pMeshCompMan->CreateComponent(pMeshComponent);

  pMeshComponent->SetMesh(hMesh);
  pMeshComponent->SetRenderPass(ezDefaultPassTypes::Foreground);

  pObject->AddComponent(pMeshComponent);
}

void ezEditorGizmoHandle::UpdateForEngine(ezWorld* pWorld)
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
}