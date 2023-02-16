#pragma once

#include <KrautFoundation/Containers/HybridArray.h>
#include <KrautFoundation/Math/Vec3.h>
#include <KrautGenerator/KrautGeneratorDLL.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct KRAUT_DLL Vertex
  {
    Vertex();

    aeVec3 m_vPosition = aeVec3::ZeroVector();
    aeVec3 m_vTexCoord = aeVec3(0, 0, 1);
    aeVec3 m_vNormal = aeVec3::ZeroVector();
    aeVec3 m_vTangent = aeVec3::ZeroVector();
    aeVec3 m_vBiTangent = aeVec3::ZeroVector();

    aeInt32 m_iSharedVertex = -1; // for internal use
    aeUInt8 m_uiColorVariation = 0;

    // index of the BranchStructure::m_Nodes of the corresponding branch data, to which this vertex belongs
    // this can be used for mapping wind or skeletal animation information from the branch structure to the mesh
    aeUInt32 m_uiBranchNodeIdx = 0xFFFFFFFF;
  };

  struct KRAUT_DLL Triangle
  {
    Triangle();

    void Flip();

    aeUInt32 m_uiVertexIDs[3] = {0, 0, 0};

    aeUInt32 m_uiPickingSubID = 0; // TODO: remove
  };

  struct KRAUT_DLL VertexRing
  {
    float m_fDiameter = 0.0f;
    aeHybridArray<aeVec3, 64> m_Vertices;
    aeHybridArray<aeInt32, 64> m_VertexIDs;
    aeHybridArray<aeVec3, 64> m_Normals;
  };

  struct KRAUT_DLL Mesh
  {
    aeUInt32 AddVertex(const Kraut::Vertex& vtx);

    aeUInt32 AddVertex(const Kraut::Vertex& vtx, aeInt32& iWriteBack);

    aeUInt32 AddVertex(const Kraut::Vertex& vtx, aeUInt32 x, aeUInt32 uiWidth, aeUInt32 y, aeUInt32 uiFirstVertex);

    void Clear();

    aeUInt32 GetNumTriangles() const;

    void GenerateVertexNormals();

    aeDeque<Kraut::Vertex> m_Vertices;
    aeDeque<Kraut::Triangle> m_Triangles;
  };

} // namespace Kraut
