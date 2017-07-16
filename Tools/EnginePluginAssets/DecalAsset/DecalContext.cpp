#include <PCH.h>
#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <Core/Graphics/Geometry.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Decals/DecalComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalContext, 1, ezRTTIDefaultAllocator<ezDecalContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Decal Asset"),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


ezDecalContext::ezDecalContext()
{
}

void ezDecalContext::OnInitialize()
{
  const char* szMeshName = "DefaultDecalPreviewMesh";
  ezMeshResourceHandle hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);

  if (!hMesh.IsValid())
  {
    const char* szMeshBufferName = "DefaultDecalPreviewMeshBuffer";

    ezMeshBufferResourceHandle hMeshBuffer;
    {
      // Build geometry
      ezGeometry geom;

      ezMat4 t, r;

      t.SetIdentity();
      geom.AddTexturedBox(ezVec3(0.5f, 1.0f, 1.0f), ezColor::White, t);

      t.SetTranslationMatrix(ezVec3(0, 1.5f, 0));
      r.SetRotationMatrixZ(ezAngle::Degree(90));
      geom.AddSphere(0.5f, 64, 64, ezColor::White, t * r);

      t.SetTranslationVector(ezVec3(0, -1.5f, 0));
      r.SetRotationMatrixY(ezAngle::Degree(90));
      geom.AddTorus(0.1f, 0.5f, 32, 64, ezColor::White, t * r);

      geom.ComputeFaceNormals();
      geom.ComputeSmoothVertexNormals();
      geom.ComputeTangents();

      ezMeshBufferResourceDescriptor desc;
      desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezGALResourceFormat::XYFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(szMeshBufferName, desc, szMeshBufferName);
    }
    {
      ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer);

      ezMeshResourceDescriptor md;
      md.UseExistingMeshBuffer(hMeshBuffer);
      md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
      //md.SetMaterial(0, "Materials/BaseMaterials/Grey.ezMaterial"); // TODO: this is actually what should be used, but it doesn't work
      md.SetMaterial(0, "{ c069b8a2-ccad-417f-bca4-8739efc222cf }");// "Materials/Common/Reference.ezMaterialAsset"
      md.ComputeBounds();

      hMesh = ezResourceManager::CreateResource<ezMeshResource>(szMeshName, md, pMeshBuffer->GetResourceDescription());
    }
  }

  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());

  ezGameObjectDesc obj;
  ezGameObject* pObj;

  // Preview Mesh that the decals get projected onto
  {
    obj.m_sName.Assign("DecalPreview");
    pWorld->CreateObject(obj, pObj);

    ezMeshComponent* pMesh;
    ezMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(hMesh);
  }

  // decals
  {
    ezStringBuilder sDecalGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sDecalGuid);

    {
      obj.m_sName.Assign("Decal1");
      obj.m_LocalPosition.Set(-0.5f, 0, 0);
      pWorld->CreateObject(obj, pObj);

      ezDecalComponent* pDecal;
      ezDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetDecalFile(sDecalGuid);
    }

    {
      obj.m_sName.Assign("Decal2");
      obj.m_LocalPosition.Set(-0.5f, -1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      ezDecalComponent* pDecal;
      ezDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetDecalFile(sDecalGuid);
    }

    {
      obj.m_sName.Assign("Decal3");
      obj.m_LocalPosition.Set(-0.5f, 1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      ezDecalComponent* pDecal;
      ezDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetDecalFile(sDecalGuid);
    }



    {
      obj.m_sName.Assign("Decal1");
      obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(180));
      obj.m_LocalPosition.Set(0.5f, 0, 0);
      pWorld->CreateObject(obj, pObj);

      ezDecalComponent* pDecal;
      ezDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(ezVec3(2));
      pDecal->SetDecalFile(sDecalGuid);
    }

    {
      obj.m_sName.Assign("Decal2");
      obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(180));
      obj.m_LocalPosition.Set(0.5f, -1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      ezDecalComponent* pDecal;
      ezDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(ezVec3(2));
      pDecal->SetDecalFile(sDecalGuid);
    }

    {
      obj.m_sName.Assign("Decal3");
      obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(180));
      obj.m_LocalPosition.Set(0.5f, 1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      ezDecalComponent* pDecal;
      ezDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(ezVec3(2));
      pDecal->SetDecalFile(sDecalGuid);
    }
  }

  // Lights
  {
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(60.0f));

    pWorld->CreateObject(obj, pObj);

    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(pObj, pDirLight);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(pObj, pAmbLight);
  }
}

ezEngineProcessViewContext* ezDecalContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezDecalViewContext, this);
}

void ezDecalContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}
