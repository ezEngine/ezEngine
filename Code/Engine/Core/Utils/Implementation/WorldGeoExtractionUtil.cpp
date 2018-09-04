#include <PCH.h>

#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Math/Mat3.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/RenderMeshComponent.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractGeometry);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractGeometry, 1, ezRTTIDefaultAllocator<ezMsgExtractGeometry>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezWorldGeoExtractionUtil::ExtractWorldGeometry(Geometry& geo, const ezWorld& world, ExtractionMode mode)
{
  EZ_LOCK(world.GetReadMarker());

  ezDeque<ezGameObjectHandle> selection;

  for (auto it = world.GetObjects(); it.IsValid(); ++it)
  {
    selection.PushBack(it->GetHandle());
  }

  ExtractWorldGeometry(geo, world, mode, selection);
}

void ezWorldGeoExtractionUtil::ExtractWorldGeometry(Geometry& geo, const ezWorld& world, ExtractionMode mode,
                                                    const ezDeque<ezGameObjectHandle>& selection)
{
  EZ_LOCK(world.GetReadMarker());

  EZ_LOG_BLOCK("ExtractWorldGeometry", world.GetName());

  ezMsgExtractGeometry msg;
  msg.m_Mode = mode;
  msg.m_pWorldGeometry = &geo;

  for (ezGameObjectHandle hObject : selection)
  {
    const ezGameObject* pObject;
    if (!world.TryGetObject(hObject, pObject))
      continue;

    pObject->SendMessage(msg);
  }
}

void ezWorldGeoExtractionUtil::WriteWorldGeometryToOBJ(const char* szFile, const Geometry& geo, const ezMat3& mTransform)
{
  EZ_LOG_BLOCK("Write World Geometry to OBJ", szFile);

  ezFileWriter file;
  if (file.Open(szFile).Failed())
  {
    ezLog::Error("Failed to open file for writing: '{0}'", szFile);
    return;
  }

  ezStringBuilder line;

  line.Format("\n\n# {0} vertices\n", geo.m_Vertices.GetCount());
  file.WriteBytes(line.GetData(), line.GetElementCount());

  for (ezUInt32 i = 0; i < geo.m_Vertices.GetCount(); ++i)
  {
    const ezVec3 pos = mTransform.TransformDirection(geo.m_Vertices[i].m_vPosition);

    line.Format("v {0} {1} {2}\n", ezArgF(pos.x, 8), ezArgF(pos.y, 8), ezArgF(pos.z, 8));

    file.WriteBytes(line.GetData(), line.GetElementCount());
  }

  line.Format("\n\n# {0} triangles\n", geo.m_Triangles.GetCount());
  file.WriteBytes(line.GetData(), line.GetElementCount());

  for (ezUInt32 i = 0; i < geo.m_Triangles.GetCount(); ++i)
  {
    line.Format("f {0} {1} {2}\n", geo.m_Triangles[i].m_uiVertexIndices[0] + 1, geo.m_Triangles[i].m_uiVertexIndices[1] + 1,
                geo.m_Triangles[i].m_uiVertexIndices[2] + 1);

    file.WriteBytes(line.GetData(), line.GetElementCount());
  }

  // TODO: write object geometry

  ezLog::Success("Wrote world geometry to '{0}'", file.GetFilePathAbsolute().GetView());
}
