#include <PCH.h>

#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractGeometry);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractGeometry, 1, ezRTTIDefaultAllocator<ezMsgExtractGeometry>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

struct GatherObjectsTraverser
{
  ezTagSet* m_pExcludeTags = nullptr;
  ezDeque<ezGameObjectHandle> m_Selection;

  ezVisitorExecution::Enum Run(ezGameObject* pObject)
  {
    if (m_pExcludeTags && pObject->GetTags().IsAnySet(*m_pExcludeTags))
      return ezVisitorExecution::Skip;

    m_Selection.PushBack(pObject->GetHandle());

    return ezVisitorExecution::Continue;
  }
};

void ezWorldGeoExtractionUtil::ExtractWorldGeometry(Geometry& geo, const ezWorld& world0, ExtractionMode mode,
                                                    ezTagSet* pExcludeTags /*= nullptr*/)
{
  // we don't modify the world, but the traverser is a non-const function
  ezWorld& world = const_cast<ezWorld&>(world0);

  EZ_LOCK(world.GetWriteMarker());

  GatherObjectsTraverser traverser;
  traverser.m_pExcludeTags = pExcludeTags;

  world.Traverse(ezMakeDelegate(&GatherObjectsTraverser::Run, &traverser), ezWorld::TraversalMethod::DepthFirst);

  ExtractWorldGeometry(geo, world, mode, traverser.m_Selection);
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

  line.Format("\n\n# {0} vertices\n\n", geo.m_Vertices.GetCount());
  file.WriteBytes(line.GetData(), line.GetElementCount());

  for (ezUInt32 i = 0; i < geo.m_Vertices.GetCount(); ++i)
  {
    const ezVec3 pos = mTransform.TransformDirection(geo.m_Vertices[i].m_vPosition);

    line.Format("v {0} {1} {2}\n", ezArgF(pos.x, 8), ezArgF(pos.y, 8), ezArgF(pos.z, 8));
    file.WriteBytes(line.GetData(), line.GetElementCount());
  }

  line.Format("\n\n# {0} triangles\n\n", geo.m_Triangles.GetCount());
  file.WriteBytes(line.GetData(), line.GetElementCount());

  const bool bFlipTriangles = ezGraphicsUtils::IsTriangleFlipRequired(mTransform);
  const char* szFaceFormat = bFlipTriangles ? "f {2} {1} {0}\n" : "f {0} {1} {2}\n";

  for (ezUInt32 i = 0; i < geo.m_Triangles.GetCount(); ++i)
  {
    const ezUInt32* indices = geo.m_Triangles[i].m_uiVertexIndices;
    line.Format(szFaceFormat, indices[0] + 1, indices[1] + 1, indices[2] + 1);

    file.WriteBytes(line.GetData(), line.GetElementCount());
  }

  ezUInt32 idxOff = geo.m_Vertices.GetCount() + 1;

  // write object geometry
  {
    line.Format("\n\n# {0} boxes\n\n", geo.m_BoxShapes.GetCount());
    file.WriteBytes(line.GetData(), line.GetElementCount());

    for (const auto& shape : geo.m_BoxShapes)
    {
      const ezVec3 he = shape.m_vHalfExtents;
      ezVec3 v[8];
      v[0] = mTransform * (shape.m_vPosition + shape.m_qRotation * ezVec3(-he.x, -he.y, -he.z));
      v[1] = mTransform * (shape.m_vPosition + shape.m_qRotation * ezVec3(-he.x, -he.y, +he.z));
      v[2] = mTransform * (shape.m_vPosition + shape.m_qRotation * ezVec3(-he.x, +he.y, -he.z));
      v[3] = mTransform * (shape.m_vPosition + shape.m_qRotation * ezVec3(-he.x, +he.y, +he.z));
      v[4] = mTransform * (shape.m_vPosition + shape.m_qRotation * ezVec3(+he.x, -he.y, -he.z));
      v[5] = mTransform * (shape.m_vPosition + shape.m_qRotation * ezVec3(+he.x, -he.y, +he.z));
      v[6] = mTransform * (shape.m_vPosition + shape.m_qRotation * ezVec3(+he.x, +he.y, -he.z));
      v[7] = mTransform * (shape.m_vPosition + shape.m_qRotation * ezVec3(+he.x, +he.y, +he.z));

      for (ezUInt32 i = 0; i < 8; ++i)
      {
        line.Format("v {0} {1} {2}\n", ezArgF(v[i].x, 8), ezArgF(v[i].y, 8), ezArgF(v[i].z, 8));
        file.WriteBytes(line.GetData(), line.GetElementCount());
      }

      // box faces
      {
        line.Format("\nf {0} {1} {2}\n", idxOff + 0, idxOff + 5, idxOff + 1);
        file.WriteBytes(line.GetData(), line.GetElementCount());

        line.Format("f {0} {1} {2}\n", idxOff + 0, idxOff + 4, idxOff + 5);
        file.WriteBytes(line.GetData(), line.GetElementCount());

        line.Format("f {0} {1} {2}\n", idxOff + 2, idxOff + 1, idxOff + 3);
        file.WriteBytes(line.GetData(), line.GetElementCount());

        line.Format("f {0} {1} {2}\n", idxOff + 2, idxOff + 0, idxOff + 1);
        file.WriteBytes(line.GetData(), line.GetElementCount());

        line.Format("f {0} {1} {2}\n", idxOff + 6, idxOff + 3, idxOff + 7);
        file.WriteBytes(line.GetData(), line.GetElementCount());

        line.Format("f {0} {1} {2}\n", idxOff + 6, idxOff + 2, idxOff + 3);
        file.WriteBytes(line.GetData(), line.GetElementCount());

        line.Format("f {0} {1} {2}\n", idxOff + 4, idxOff + 7, idxOff + 5);
        file.WriteBytes(line.GetData(), line.GetElementCount());

        line.Format("f {0} {1} {2}\n", idxOff + 4, idxOff + 6, idxOff + 7);
        file.WriteBytes(line.GetData(), line.GetElementCount());

        line.Format("f {0} {1} {2}\n", idxOff + 4, idxOff + 2, idxOff + 6);
        file.WriteBytes(line.GetData(), line.GetElementCount());

        line.Format("f {0} {1} {2}\n", idxOff + 4, idxOff + 0, idxOff + 2);
        file.WriteBytes(line.GetData(), line.GetElementCount());

        line.Format("f {0} {1} {2}\n", idxOff + 7, idxOff + 1, idxOff + 5);
        file.WriteBytes(line.GetData(), line.GetElementCount());

        line.Format("f {0} {1} {2}\n\n\n", idxOff + 7, idxOff + 3, idxOff + 1);
        file.WriteBytes(line.GetData(), line.GetElementCount());
      }

      idxOff += 8;
    }
  }

  ezLog::Success("Wrote world geometry to '{0}'", file.GetFilePathAbsolute().GetView());
}



EZ_STATICLINK_FILE(Core, Core_Utils_Implementation_WorldGeoExtractionUtil);

