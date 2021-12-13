#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractGeometry);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractGeometry, 1, ezRTTIDefaultAllocator<ezMsgExtractGeometry>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezWorldGeoExtractionUtil::ExtractWorldGeometry(MeshObjectList& objects, const ezWorld& world, ExtractionMode mode, ezTagSet* pExcludeTags /*= nullptr*/)
{
  EZ_PROFILE_SCOPE("ExtractWorldGeometry");
  EZ_LOG_BLOCK("ExtractWorldGeometry", world.GetName());

  ezMsgExtractGeometry msg;
  msg.m_Mode = mode;
  msg.m_pMeshObjects = &objects;

  EZ_LOCK(world.GetReadMarker());

  for (auto it = world.GetObjects(); it.IsValid(); ++it)
  {
    if (pExcludeTags != nullptr && it->GetTags().IsAnySet(*pExcludeTags))
      continue;

    it->SendMessage(msg);
  }
}

void ezWorldGeoExtractionUtil::ExtractWorldGeometry(MeshObjectList& objects, const ezWorld& world, ExtractionMode mode, const ezDeque<ezGameObjectHandle>& selection)
{
  EZ_PROFILE_SCOPE("ExtractWorldGeometry");
  EZ_LOG_BLOCK("ExtractWorldGeometry", world.GetName());

  ezMsgExtractGeometry msg;
  msg.m_Mode = mode;
  msg.m_pMeshObjects = &objects;

  EZ_LOCK(world.GetReadMarker());

  for (ezGameObjectHandle hObject : selection)
  {
    const ezGameObject* pObject;
    if (!world.TryGetObject(hObject, pObject))
      continue;

    pObject->SendMessage(msg);
  }
}

void ezWorldGeoExtractionUtil::WriteWorldGeometryToOBJ(const char* szFile, const MeshObjectList& objects, const ezMat3& mTransform)
{
  EZ_LOG_BLOCK("Write World Geometry to OBJ", szFile);

  ezFileWriter file;
  if (file.Open(szFile).Failed())
  {
    ezLog::Error("Failed to open file for writing: '{0}'", szFile);
    return;
  }

  ezMat4 transform = ezMat4::IdentityMatrix();
  transform.SetRotationalPart(mTransform);

  ezStringBuilder line;

  line = "\n\n# vertices\n\n";
  file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

  ezUInt32 uiVertexOffset = 0;
  ezDeque<ezUInt32> indices;

  for (const MeshObject& object : objects)
  {
    ezResourceLock<ezCpuMeshResource> pCpuMesh(object.m_hMeshResource, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
    {
      continue;
    }

    const auto& meshBufferDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

    const ezVec3* pPositions = nullptr;
    ezUInt32 uiElementStride = 0;
    if (ezMeshBufferUtils::GetPositionStream(meshBufferDesc, pPositions, uiElementStride).Failed())
    {
      continue;
    }

    ezMat4 finalTransform = transform * object.m_GlobalTransform.GetAsMat4();

    // write out all vertices
    for (ezUInt32 i = 0; i < meshBufferDesc.GetVertexCount(); ++i)
    {
      const ezVec3 pos = finalTransform.TransformPosition(*pPositions);

      line.Format("v {0} {1} {2}\n", ezArgF(pos.x, 8), ezArgF(pos.y, 8), ezArgF(pos.z, 8));
      file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

      pPositions = ezMemoryUtils::AddByteOffset(pPositions, uiElementStride);
    }

    // collect all indices
    bool flip = ezGraphicsUtils::IsTriangleFlipRequired(finalTransform.GetRotationalPart());
    if (meshBufferDesc.Uses32BitIndices())
    {
      const ezUInt32* pTypedIndices = reinterpret_cast<const ezUInt32*>(meshBufferDesc.GetIndexBufferData().GetPtr());

      for (ezUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
      {
        indices.PushBack(pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset);
        indices.PushBack(pTypedIndices[p * 3 + 1] + uiVertexOffset);
        indices.PushBack(pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset);
      }
    }
    else
    {
      const ezUInt16* pTypedIndices = reinterpret_cast<const ezUInt16*>(meshBufferDesc.GetIndexBufferData().GetPtr());

      for (ezUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
      {
        indices.PushBack(pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset);
        indices.PushBack(pTypedIndices[p * 3 + 1] + uiVertexOffset);
        indices.PushBack(pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset);
      }
    }

    uiVertexOffset += meshBufferDesc.GetVertexCount();
  }

  line = "\n\n# triangles\n\n";
  file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

  for (ezUInt32 i = 0; i < indices.GetCount(); i += 3)
  {
    // indices are 1 based in obj
    line.Format("f {0} {1} {2}\n", indices[i + 0] + 1, indices[i + 1] + 1, indices[i + 2] + 1);
    file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();
  }

  ezLog::Success("Wrote world geometry to '{0}'", file.GetFilePathAbsolute().GetView());
}

//////////////////////////////////////////////////////////////////////////

void ezMsgExtractGeometry::AddMeshObject(const ezTransform& transform, ezCpuMeshResourceHandle hMeshResource)
{
  m_pMeshObjects->PushBack({transform, hMeshResource});
}

void ezMsgExtractGeometry::AddBox(const ezTransform& transform, ezVec3 vExtents)
{
  const char* szResourceName = "CpuMesh-UnitBox";
  ezCpuMeshResourceHandle hBoxMesh = ezResourceManager::GetExistingResource<ezCpuMeshResource>(szResourceName);
  if (hBoxMesh.IsValid() == false)
  {
    ezGeometry geom;
    geom.AddBox(ezVec3(1), ezColor::White);
    geom.TriangulatePolygons();
    geom.ComputeTangents();

    ezMeshResourceDescriptor desc;
    desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Data/Base/Materials/Common/Pattern.ezMaterialAsset

    desc.MeshBufferDesc().AddCommonStreams();
    desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

    desc.ComputeBounds();

    hBoxMesh = ezResourceManager::GetOrCreateResource<ezCpuMeshResource>(szResourceName, std::move(desc), szResourceName);
  }

  auto& meshObject = m_pMeshObjects->ExpandAndGetRef();
  meshObject.m_GlobalTransform = transform;
  meshObject.m_GlobalTransform.m_vScale *= vExtents;
  meshObject.m_hMeshResource = hBoxMesh;
}

EZ_STATICLINK_FILE(Core, Core_Utils_Implementation_WorldGeoExtractionUtil);
