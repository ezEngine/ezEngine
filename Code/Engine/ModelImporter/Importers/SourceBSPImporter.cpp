#include <PCH.h>
#include <ModelImporter/Importers/SourceBSPImporter.h>
#include <ModelImporter/Scene.h>
#include <ModelImporter/Node.h>
#include <ModelImporter/Mesh.h>
#include <ModelImporter/VertexData.h>
#include <ModelImporter/Material.h>
#include <Foundation/IO/FileSystem/FileReader.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Containers/DynamicArray.h>


namespace SourceBSP
{
  enum LumpTypes
  {
    LUMP_ENTITIES = 0,	// *
    LUMP_PLANES = 1,	// *
    LUMP_TEXDATA = 2,	// *
    LUMP_VERTEXES = 3,	// *
    LUMP_VISIBILITY = 4,	// *
    LUMP_NODES = 5,	// *
    LUMP_TEXINFO = 6,	// *
    LUMP_FACES = 7,	// *
    LUMP_LIGHTING = 8,	// *
    LUMP_OCCLUSION = 9,
    LUMP_LEAFS = 10,	// *
    LUMP_FACEIDS = 11,
    LUMP_EDGES = 12,	// *
    LUMP_SURFEDGES = 13,	// *
    LUMP_MODELS = 14,	// *
    LUMP_WORLDLIGHTS = 15,	//
    LUMP_LEAFFACES = 16,	// *
    LUMP_LEAFBRUSHES = 17,	// *
    LUMP_BRUSHES = 18,	// *
    LUMP_BRUSHSIDES = 19,	// *
    LUMP_AREAS = 20,	// *
    LUMP_AREAPORTALS = 21,	// *
    LUMP_UNUSED0 = 22,
    LUMP_UNUSED1 = 23,
    LUMP_UNUSED2 = 24,
    LUMP_UNUSED3 = 25,
    LUMP_DISPINFO = 26,
    LUMP_ORIGINALFACES = 27,
    LUMP_PHYSDISP = 28,
    LUMP_PHYSCOLLIDE = 29,
    LUMP_VERTNORMALS = 30,
    LUMP_VERTNORMALINDICES = 31,
    LUMP_DISP_LIGHTMAP_ALPHAS = 32,
    LUMP_DISP_VERTS = 33,		// CDispVerts
    LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS = 34,	// For each displacement
                                              //     For each lightmap sample
                                              //         byte for index
                                              //         if 255, then index = next byte + 255
                                              //         3 bytes for barycentric coordinates
    // The game lump is a method of adding game-specific lumps
    // FIXME: Eventually, all lumps could use the game lump system
    LUMP_GAME_LUMP = 35,
    LUMP_LEAFWATERDATA = 36,
    LUMP_PRIMITIVES = 37,
    LUMP_PRIMVERTS = 38,
    LUMP_PRIMINDICES = 39,
    // A pak file can be embedded in a .bsp now, and the file system will search the pak
    //  file first for any referenced names, before deferring to the game directory
    //  file system/pak files and finally the base directory file system/pak files.
    LUMP_PAKFILE = 40,
    LUMP_CLIPPORTALVERTS = 41,
    // A map can have a number of cubemap entities in it which cause cubemap renders
    // to be taken after running vrad.
    LUMP_CUBEMAPS = 42,
    LUMP_TEXDATA_STRING_DATA = 43,
    LUMP_TEXDATA_STRING_TABLE = 44,
    LUMP_OVERLAYS = 45,
    LUMP_LEAFMINDISTTOWATER = 46,
    LUMP_FACE_MACRO_TEXTURE_INFO = 47,
    LUMP_DISP_TRIS = 48,
    LUMP_PHYSCOLLIDESURFACE = 49,	// deprecated.  We no longer use win32-specific havok compression on terrain
    LUMP_WATEROVERLAYS = 50,
    LUMP_LEAF_AMBIENT_INDEX_HDR = 51,	// index of LUMP_LEAF_AMBIENT_LIGHTING_HDR
    LUMP_LEAF_AMBIENT_INDEX = 52,	// index of LUMP_LEAF_AMBIENT_LIGHTING

    // optional lumps for HDR
    LUMP_LIGHTING_HDR = 53,
    LUMP_WORLDLIGHTS_HDR = 54,
    LUMP_LEAF_AMBIENT_LIGHTING_HDR = 55,	// NOTE: this data overrides part of the data stored in LUMP_LEAFS.
    LUMP_LEAF_AMBIENT_LIGHTING = 56,	// NOTE: this data overrides part of the data stored in LUMP_LEAFS.

    LUMP_XZIPPAKFILE = 57,   // deprecated. xbox 1: xzip version of pak file
    LUMP_FACES_HDR = 58,	// HDR maps may have different face data.
    LUMP_MAP_FLAGS = 59,   // extended level-wide flags. not present in all levels
    LUMP_OVERLAY_FADES = 60,	// Fade distances for overlays
  };

  const int VBSP_HEADER = (('P' << 24) + ('S' << 16) + ('B' << 8) + 'V');

  const int HEADER_LUMPS = 64;

  struct Lump_t
  {
    ezInt32 fileofs;
    ezInt32 filelen;
    ezInt32 version;
    ezUInt8 fourCC[4];

    bool isUsed() const
    {
      return fileofs != 0 || filelen != 0;
    }
  };

  struct Header_t
  {
    int ident;
    int version;
    Lump_t lumps[HEADER_LUMPS];
    int mapRevision;
  };

  enum TexInfoFlags_e
  {
    SURF_LIGHT = 0x0001, // value will hold the light strength
    SURF_SKY2D = 0x0002, // don't draw, indicates we should skylight + draw 2d sky but not draw the 3D skybox
    SURF_SKY = 0x0004, // don't draw, but add to skybox
    SURF_WARP = 0x0008, // turbulent water warp
    SURF_TRANS = 0x0010,
    SURF_NOPORTAL = 0x0020, // the surface can not have a portal placed on it
    SURF_TRIGGER = 0x0040, // FIXME: This is an xbox hack to work around elimination of trigger surfaces, which breaks occluders
    SURF_NODRAW = 0x0080, // don't bother referencing the texture
    SURF_HINT = 0x0100, // make a primary bsp splitter
    SURF_SKIP = 0x0200, // completely ignore, allowing non-closed brushes
    SURF_NOLIGHT = 0x0400, // Don't calculate light
    SURF_BUMPLIGHT = 0x0800, // calculate three lightmaps for the surface for bumpmapping
    SURF_NOSHADOWS = 0x1000, // Don't receive shadows
    SURF_NODECALS = 0x2000, // Don't receive decals
    SURF_NOCHOP = 0x4000, // Don't subdivide patches on this surface
    SURF_HITBOX = 0x8000 // surface is part of a hitbox
  };

  struct Vertex_t
  {
    float x;
    float y;
    float z;

    Vertex_t()
      : x(0), y(0), z(0)
    {}

    Vertex_t(float _x, float _y, float _z)
      : x(_x), y(_y), z(_z)
    {}

    bool operator == (const Vertex_t& other) const
    {
      const float xDiff = x - other.x;
      const float yDiff = y - other.y;
      const float zDiff = z - other.z;

      const float epsilon = 0.01f;

      return ezMath::Abs(xDiff) < epsilon && ezMath::Abs(yDiff) < epsilon && ezMath::Abs(zDiff) < epsilon;
    }

    bool operator != (const Vertex_t& other) const
    {
      return !(*this == other);
    }
  };

  struct Plane_t
  {
    Vertex_t normal; // normal vector
    float dist; // distance from origin
    ezInt32 type; // plane axis identifier
  };

  struct Edge_t
  {
    ezUInt16 v[2]; // vertex indices
  };

  struct Face_t
  {
    ezUInt16 planenum; // the plane number
    ezUInt8 side; // faces opposite to the node's plane direction
    ezUInt8 onNode; // 1 of on node, 0 if in leaf
    ezInt32 firstedge; // index into surfedges
    ezInt16 numedges; // number of surfedges
    ezInt16 texinfo; // texture info
    ezInt16 dispinfo; // displacement info
    ezInt16 surfaceFogVolumeID; // ?
    ezUInt8 styles[4]; // switchable lighting info
    ezInt32 lightofs; // offset into lightmap lump
    float area; // face area in units^2
    ezInt32 LightmapTextureMinsInLuxels[2]; // texture lighting info
    ezInt32 LightmapTextureSizeInLuxels[2]; // texture lighting info
    ezInt32 origFace; // original face this was split from
    ezUInt16 numPrims; // primitives
    ezUInt16 firstPrimID;
    ezUInt32 smoothingGroups; // lightmap smoothing group
  };

  struct TexInfo_t
  {
    float textureVecsTexelsPerWorldUnits[2][4]; // [s/t][xyz offset]
    float lightmapVecsLuxelsPerWorldUnits[2][4]; // [s/t][xyz offset] - length is in units of texels/area
    ezInt32 flags; // miptex flags + overrides
    ezInt32 texdata; // Pointer to texture name, size, etc.
  };

  struct TexData_t
  {
    Vertex_t reflectivity;
    ezInt32 nameStringTableID; // index into g_StringTable for the texture name
    ezInt32 width, height; // source image
    ezInt32 view_width, view_height; //
  };

  struct Brush_t
  {
    ezInt32 firstside;
    ezInt32 numsides;
    ezInt32 contents;
  };

  struct BrushSide_t
  {
    ezUInt16 planenum; // facing out of the leaf
    ezInt16 texinfo;
    ezInt16 dispinfo; // displacement info (BSPVERSION 7)
    ezInt16 bevel; // is the side a bevel plane? (BSPVERSION 7)
  };

  struct CDispSubNeighbor
  {
    ezUInt16 m_iNeighbor; // This indexes into ddispinfos.
                     // 0xFFFF if there is no neighbor here.

    ezUInt8 m_NeighborOrientation; // (CCW) rotation of the neighbor wrt this displacement.

                              // These use the NeighborSpan type.
    ezUInt8 m_Span; // Where the neighbor fits onto this side of our displacement.
    ezUInt8 m_NeighborSpan; // Where we fit onto our neighbor.
  };

  struct CDispNeighbor
  {
    CDispSubNeighbor m_SubNeighbors[2];
  };

  struct CDispCornerNeighbors
  {
    ezUInt16 m_Neighbors[4]; // indices of neighbors.
    ezUInt8 m_nNeighbors;
  };

  struct DispInfo_t
  {
    Vertex_t startPosition; // start position used for orientation
    ezInt32 DispVertStart; // Index into LUMP_DISP_VERTS.
    ezInt32 DispTriStart; // Index into LUMP_DISP_TRIS.
    ezInt32 power; // power - indicates size of surface (2^power	1)
    ezInt32 minTess; // minimum tesselation allowed
    float smoothingAngle; // lighting smoothing angle
    ezInt32 contents; // surface contents
    ezUInt16 MapFace; // Which map face this displacement comes from.
    ezInt32 LightmapAlphaStart; // Index into ddisplightmapalpha.
    ezInt32 LightmapSamplePositionStart; // Index into LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS.
    CDispNeighbor EdgeNeighbors[4]; // Indexed by NEIGHBOREDGE_ defines.
    CDispCornerNeighbors CornerNeighbors[4]; // Indexed by CORNER_ defines.
    ezUInt32 AllowedVerts[10]; // active verticies
  };

  struct DispVertex_t
  {
    Vertex_t vector;
    float m_distance;
    float m_alpha;
  };

  struct File
  {
    File(ezArrayPtr<ezUInt8> memory);

    Header_t* header;

    Vertex_t* vertices;
    ezUInt32 numVertices;

    Plane_t* planes;
    ezUInt32 numPlanes;

    Edge_t* edges;
    ezUInt32 numEdges;

    Face_t* faces;
    ezUInt32 numFaces;

    TexInfo_t* texInfos;
    ezUInt32 numTexInfos;

    TexData_t* texDatas;
    ezUInt32 numTexDatas;

    Brush_t* brushes;
    ezUInt32 numBrushes;

    BrushSide_t* brushSides;
    ezUInt32 numBrushSides;

    ezInt32* surfEdges;
    ezUInt32 numSurfEdges;


    char* texDataStrings;
    ezInt32* texDataStringOffsets;
    ezUInt32 numTexDataStringOffsets;

    char* entityData;

    DispInfo_t* dispInfos;
    ezUInt32 numDispInfos;

    DispVertex_t* dispVertices;
    ezUInt32 numDispVertices;

    const char* getTexDataString(ezUInt32 index) const;

    bool m_valid;
  };

  File::File(ezArrayPtr<ezUInt8> fileContent)
    : m_valid(false)
  {
    if (fileContent.GetCount() < static_cast<ezUInt32>(sizeof(Header_t)))
    {
      m_valid = false;
      return;
    }

    ezUInt8* memory = fileContent.GetPtr();

    header = reinterpret_cast<Header_t*>(memory);

    if (header->ident != SourceBSP::VBSP_HEADER)
    {
      m_valid = false;
      return;
    }

    vertices = reinterpret_cast<Vertex_t*>(memory + header->lumps[LUMP_VERTEXES].fileofs);
    numVertices = header->lumps[LUMP_VERTEXES].filelen / sizeof(Vertex_t);

    planes = reinterpret_cast<Plane_t*>(memory + header->lumps[LUMP_PLANES].fileofs);
    numPlanes = header->lumps[LUMP_PLANES].filelen / sizeof(Plane_t);

    edges = reinterpret_cast<Edge_t*>(memory + header->lumps[LUMP_EDGES].fileofs);
    numEdges = header->lumps[LUMP_EDGES].filelen / sizeof(Edge_t);

    faces = reinterpret_cast<Face_t*>(memory + header->lumps[LUMP_FACES].fileofs);
    numFaces = header->lumps[LUMP_FACES].filelen / sizeof(Face_t);

    texInfos = reinterpret_cast<TexInfo_t*>(memory + header->lumps[LUMP_TEXINFO].fileofs);
    numTexInfos = header->lumps[LUMP_TEXINFO].filelen / sizeof(TexInfo_t);

    texDatas = reinterpret_cast<TexData_t*>(memory + header->lumps[LUMP_TEXDATA].fileofs);
    numTexDatas = header->lumps[LUMP_TEXDATA].filelen / sizeof(TexData_t);

    brushes = reinterpret_cast<Brush_t*>(memory + header->lumps[LUMP_BRUSHES].fileofs);
    numBrushes = header->lumps[LUMP_BRUSHES].filelen / sizeof(Brush_t);

    brushSides = reinterpret_cast<BrushSide_t*>(memory + header->lumps[LUMP_BRUSHSIDES].fileofs);
    numBrushSides = header->lumps[LUMP_BRUSHSIDES].filelen / sizeof(BrushSide_t);

    surfEdges = reinterpret_cast<ezInt32*>(memory + header->lumps[LUMP_SURFEDGES].fileofs);
    numSurfEdges = header->lumps[LUMP_SURFEDGES].filelen / sizeof(ezInt32);

    /*
    char* texData;
    i32* texDataOffsets;
    u32 numTexDataOffsets;*/

    texDataStrings = reinterpret_cast<char*>(memory + header->lumps[LUMP_TEXDATA_STRING_DATA].fileofs);
    texDataStringOffsets = reinterpret_cast<ezInt32*>(memory + header->lumps[LUMP_TEXDATA_STRING_TABLE].fileofs);
    numTexDataStringOffsets = header->lumps[LUMP_TEXDATA_STRING_TABLE].filelen / sizeof(ezInt32);


    entityData = reinterpret_cast<char*>(memory + header->lumps[LUMP_ENTITIES].fileofs);

    dispInfos = reinterpret_cast<DispInfo_t*>(memory + header->lumps[LUMP_DISPINFO].fileofs);
    numDispInfos = header->lumps[LUMP_DISPINFO].filelen / sizeof(DispInfo_t);

    dispVertices = reinterpret_cast<DispVertex_t*>(memory + header->lumps[LUMP_DISP_VERTS].fileofs);
    numDispVertices = header->lumps[LUMP_DISP_VERTS].filelen / sizeof(DispVertex_t);

    m_valid = true;
  }

  const char* File::getTexDataString(ezUInt32 index) const
  {
    EZ_ASSERT_ALWAYS(index < numTexDataStringOffsets, "BSP file tex data string out of bounds.");

    return texDataStrings + texDataStringOffsets[index];
  }


  void calculateUV(const SourceBSP::Vertex_t& worldPos, const SourceBSP::TexInfo_t& texInfo, const SourceBSP::TexData_t& texData, float& u, float& v)
  {
    u = worldPos.x * texInfo.textureVecsTexelsPerWorldUnits[0][0] + worldPos.y * texInfo.textureVecsTexelsPerWorldUnits[0][1] + worldPos.z * texInfo.textureVecsTexelsPerWorldUnits[0][2] + texInfo.textureVecsTexelsPerWorldUnits[0][3];
    v = worldPos.x * texInfo.textureVecsTexelsPerWorldUnits[1][0] + worldPos.y * texInfo.textureVecsTexelsPerWorldUnits[1][1] + worldPos.z * texInfo.textureVecsTexelsPerWorldUnits[1][2] + texInfo.textureVecsTexelsPerWorldUnits[1][3];

    u /= texData.width;
    v /= texData.height;
  }

  struct TempVertex
  {
    EZ_DECLARE_POD_TYPE();

    float x, y, z;
    float nx, ny, nz;
    float u, v;
  };

  // 1 meter equals 64 units in Source (default player height is 96 units)
  constexpr float bspToMetricScale = 1.0f / 64.0f;

  ezResult ConvertBSPGeometryToMesh(SourceBSP::File& bspFile, ezModelImporter::Mesh* pMesh, ezModelImporter::Scene* pScene)
  {
    ezMap<ezString, ezModelImporter::MaterialHandle> alreadyCreatedMaterials;

    ezDynamicArray<ezUInt32> indices;
    ezDynamicArray<TempVertex> vertices;

    for (ezUInt32 faceIndex = 0; faceIndex < bspFile.numFaces; ++faceIndex)
    {
      const SourceBSP::Face_t& current = bspFile.faces[faceIndex];

      SourceBSP::Plane_t& plane = bspFile.planes[current.planenum];

      if (current.texinfo == -1)
        continue;

      SourceBSP::TexInfo_t& texInfo = bspFile.texInfos[current.texinfo];
      SourceBSP::TexData_t& texData = bspFile.texDatas[texInfo.texdata];


      // Skip triggers, no draw and sky surfaces
      if (
        (texInfo.flags & SourceBSP::SURF_NODRAW)
        || (texInfo.flags & SourceBSP::SURF_SKIP)
        || (texInfo.flags & SourceBSP::SURF_HINT)
        || (texInfo.flags & SourceBSP::SURF_HITBOX)
        || (texInfo.flags & SourceBSP::SURF_SKY)
        || (texInfo.flags & SourceBSP::SURF_SKY2D)
        || (texInfo.flags & SourceBSP::SURF_TRIGGER)
        )
        continue;


      const char* material = bspFile.getTexDataString(texInfo.texdata);

      if (!strcmp(material, "TOOLS/TOOLSCLIP"))
        continue;

      // TODO: Consider importing this for collision geometry
      if (!strcmp(material, "TOOLS/TOOLSPLAYERCLIP"))
        continue;

      if (!strcmp(material, "TOOLS/FOGVOLUME"))
        continue;

      ezModelImporter::MaterialHandle materialHandle;

      if (alreadyCreatedMaterials.Contains(material))
      {
        materialHandle = *alreadyCreatedMaterials.GetValue(material);
      }
      else
      {
        ezUniquePtr<ezModelImporter::Material> newMat(EZ_DEFAULT_NEW(ezModelImporter::Material));
        newMat->m_Name = material;

        materialHandle = pScene->AddMaterial(std::move(newMat));

        alreadyCreatedMaterials.Insert(material, materialHandle);
      }

      ezUInt32 partFirstIndex = vertices.GetCount();
      ezUInt32 indexOffset = indices.GetCount();

      // No displacement surface? Create a bunch of triangles from the edges
      if (current.dispinfo == -1)
      {
        // Build vertices
        for (ezInt32 edgeIndex = current.firstedge; edgeIndex < current.firstedge + (current.numedges - 1); ++edgeIndex)
        {
          ezInt32 surfEdge = bspFile.surfEdges[edgeIndex];

          SourceBSP::Edge_t& edge = bspFile.edges[(surfEdge < 0) ? -surfEdge : surfEdge];

          SourceBSP::Vertex_t& v1 = bspFile.vertices[edge.v[(surfEdge < 0) ? 1 : 0]];
          SourceBSP::Vertex_t& v2 = bspFile.vertices[edge.v[(surfEdge < 0) ? 0 : 1]];

          TempVertex vert;
          vert.x = v1.x * bspToMetricScale; vert.y = v1.z * bspToMetricScale; vert.z = v1.y * bspToMetricScale;
          vert.nx = plane.normal.x; vert.ny = plane.normal.z; vert.nz = plane.normal.y;

          calculateUV(v1, texInfo, texData, vert.u, vert.v);

          // Only add first vertex for first edge, other edges reuse the previous vertex
          if (edgeIndex == current.firstedge)
          {
            vertices.PushBack(vert);
          }

          vert.x = v2.x * bspToMetricScale; vert.y = v2.z * bspToMetricScale; vert.z = v2.y * bspToMetricScale;
          calculateUV(v2, texInfo, texData, vert.u, vert.v);

          vertices.PushBack(vert);
        }

        // Build indices for face
        for (ezInt32 edge = 1; edge < current.numedges - 1; ++edge)
        {
          indices.PushBack(partFirstIndex);
          indices.PushBack(partFirstIndex + edge);
          indices.PushBack(partFirstIndex + edge + 1);
        }

        ezModelImporter::SubMesh subMesh;
        subMesh.m_Material = materialHandle;
        subMesh.m_uiFirstTriangle = indexOffset / 3;
        subMesh.m_uiTriangleCount = (current.numedges - 2);

        pMesh->AddSubMesh(subMesh);
      }
      else
      {
        SourceBSP::DispInfo_t& displacementInfo = bspFile.dispInfos[current.dispinfo];

        // This should never happen - but just to be on the safe side
        if (current.numedges != 4)
          continue;

        // Each edge has 2 ^ power + 1 vertices
        ezUInt32 numVerticesPerEdge = ((1u << displacementInfo.power) + 1);
        ezUInt32 numVertices = numVerticesPerEdge * numVerticesPerEdge;

        // Get the corner vertices for the original face
        // This is the base of the displacement vertices later on
        SourceBSP::Vertex_t cornerVertices[4];

        for (ezInt32 edge = 0; edge < 4; ++edge)
        {
          ezInt32 surfEdge = bspFile.surfEdges[current.firstedge + edge];
          SourceBSP::Edge_t& dispEdge = bspFile.edges[(surfEdge < 0) ? -surfEdge : surfEdge];

          cornerVertices[edge] = bspFile.vertices[dispEdge.v[(surfEdge < 0) ? 1 : 0]];
        }

        // If the first corner vertex is not equal to the start position we need to search the original corner vertex
        if (cornerVertices[0] != displacementInfo.startPosition)
        {
          ezInt32 offset = 0;
          for (ezInt32 vertIdx = 1; vertIdx < 4; ++vertIdx)
          {
            if (cornerVertices[vertIdx] == displacementInfo.startPosition)
            {
              offset = vertIdx;
              break;
            }
          }

          SourceBSP::Vertex_t origCornerVertices[4];
          ezMemoryUtils::Copy(origCornerVertices, cornerVertices, 4);

          for (ezInt32 vertIdx = 0; vertIdx < 4; ++vertIdx)
          {
            cornerVertices[vertIdx] = origCornerVertices[(vertIdx + offset) % 4];
          }
        }

        ezVec3 uvs[4];

        for (int i = 0; i < 4; ++i)
        {
          float u, v;
          calculateUV(cornerVertices[i], texInfo, texData, u, v);
          uvs[i].Set(u, v, 0.0f);
        }

        ezVec3 xStep_03 = (ezVec3(cornerVertices[3].x, cornerVertices[3].y, cornerVertices[3].z) - ezVec3(cornerVertices[0].x, cornerVertices[0].y, cornerVertices[0].z));
        xStep_03 /= float(numVerticesPerEdge - 1);

        ezVec3 uvStep_03 = uvs[3] - uvs[1];
        uvStep_03 /= float(numVerticesPerEdge - 1);

        ezVec3 xStep_12 = (ezVec3(cornerVertices[2].x, cornerVertices[2].y, cornerVertices[2].z) - ezVec3(cornerVertices[1].x, cornerVertices[1].y, cornerVertices[1].z));
        xStep_12 /= float(numVerticesPerEdge - 1);

        ezVec3 uvStep_12 = uvs[2] - uvs[1];
        uvStep_12 /= float(numVerticesPerEdge - 1);

        for (ezUInt32 y = 0; y < numVerticesPerEdge; ++y)
        {
          for (ezUInt32 x = 0; x < numVerticesPerEdge; ++x)
          {
            ezVec3 x03 = ezVec3(cornerVertices[0].x, cornerVertices[0].y, cornerVertices[0].z) + xStep_03 * (float)x;
            ezVec3 x12 = ezVec3(cornerVertices[1].x, cornerVertices[1].y, cornerVertices[1].z) + xStep_12 * (float)x;

            ezVec3 currentBase = x03 + ((x12 - x03) * ((float)y / (numVerticesPerEdge - 1)));

            ezUInt32 linearIndex = y * numVerticesPerEdge + x;

            ezVec3 offset = ezVec3(bspFile.dispVertices[displacementInfo.DispVertStart + linearIndex].vector.x, bspFile.dispVertices[displacementInfo.DispVertStart + linearIndex].vector.y, bspFile.dispVertices[displacementInfo.DispVertStart + linearIndex].vector.z);
            offset *= bspFile.dispVertices[displacementInfo.DispVertStart + linearIndex].m_distance;

            currentBase += offset;

            ezVec3 uv03 = uvs[0] + uvStep_03 * (float)x;
            ezVec3 uv12 = uvs[1] + uvStep_12 * (float)x;

            ezVec3 uv = uv03 + ((uv12 - uv03) * ((float)y / (numVerticesPerEdge - 1)));

            TempVertex vert;
            vert.x = currentBase.x * bspToMetricScale; vert.y = currentBase.z * bspToMetricScale; vert.z = currentBase.y * bspToMetricScale;
            vert.nx = plane.normal.x; vert.ny = plane.normal.z; vert.nz = plane.normal.y;
            vert.u = uv.x; vert.v = uv.y;

            vertices.PushBack(vert);
          }
        }

        // Build triangles from these vertices
        for (ezUInt32 x = 1; x < numVerticesPerEdge; ++x)
        {
          for (ezUInt32 y = 1; y < numVerticesPerEdge; ++y)
          {
            indices.PushBack(y * numVerticesPerEdge + x + partFirstIndex);
            indices.PushBack((y - 1) * numVerticesPerEdge + x + partFirstIndex);
            indices.PushBack((y - 1) * numVerticesPerEdge + (x - 1) + partFirstIndex);

            indices.PushBack(y * numVerticesPerEdge + x + partFirstIndex);
            indices.PushBack((y - 1) * numVerticesPerEdge + (x - 1) + partFirstIndex);
            indices.PushBack(y * numVerticesPerEdge + (x - 1) + partFirstIndex);

          }
        }

        ezModelImporter::SubMesh subMesh;
        subMesh.m_Material = materialHandle;
        subMesh.m_uiFirstTriangle = indexOffset / 3;
        subMesh.m_uiTriangleCount = 2 * (numVerticesPerEdge - 1) * (numVerticesPerEdge - 1);

        pMesh->AddSubMesh(subMesh);
      }
    }

    if ((indices.GetCount() % 3) != 0)
    {
      ezLog::Error("Index count of BSP import are not divisible by 3, can't build triangles.");
      return EZ_FAILURE;
    }

    // Add vertices and triangles
    ezHybridArray<ezModelImporter::VertexDataStream*, 3> streams;

    auto positionDataStream = pMesh->GetDataStream(ezGALVertexAttributeSemantic::Position);
    auto normalDataStream = pMesh->GetDataStream(ezGALVertexAttributeSemantic::Normal);
    auto texCoordDataStream = pMesh->GetDataStream(ezGALVertexAttributeSemantic::TexCoord0);

    streams.PushBack(positionDataStream);
    streams.PushBack(normalDataStream);
    streams.PushBack(texCoordDataStream);

    for (auto& vertex : vertices)
    {
      positionDataStream->AddValues(ezArrayPtr<char>(reinterpret_cast<char*>(&vertex.x), 3 * sizeof(float)));
      normalDataStream->AddValues(ezArrayPtr<char>(reinterpret_cast<char*>(&vertex.nx), 3 * sizeof(float)));
      texCoordDataStream->AddValues(ezArrayPtr<char>(reinterpret_cast<char*>(&vertex.u), 2 * sizeof(float)));
    }

    pMesh->AddTriangles(indices.GetCount() / 3);

    ezArrayPtr<ezModelImporter::Mesh::Triangle> triangleList = pMesh->GetTriangles();
    for (ezModelImporter::VertexDataStream* stream : streams)
    {
      ezUInt32 uiAttributeSize = stream->GetAttributeSize();
      for (ezUInt32 i = 0; i < triangleList.GetCount(); ++i)
      {
        stream->SetDataIndex(triangleList[i].m_Vertices[0], indices[i * 3 + 0] * uiAttributeSize);
        stream->SetDataIndex(triangleList[i].m_Vertices[1], indices[i * 3 + 1] * uiAttributeSize);
        stream->SetDataIndex(triangleList[i].m_Vertices[2], indices[i * 3 + 2] * uiAttributeSize);
      }
    }

    return EZ_SUCCESS;
  }
}


namespace ezModelImporter
{
  SourceBSPImporter::SourceBSPImporter()
  {
    m_supportedFileFormats.PushBack("bsp");
  }

  ezArrayPtr<const ezString> SourceBSPImporter::GetSupportedFileFormats() const
  {
    return ezMakeArrayPtr(m_supportedFileFormats);
  }


  ezSharedPtr<Scene> SourceBSPImporter::ImportScene(const char* szFileName, ezBitflags<ImportFlags> importFlags)
  {
    ezDynamicArray<ezUInt8> fileContent;
    fileContent.Reserve(1024 * 1024);

    // Read the whole file into memory since we map BSP data structures directly to memory content
    {
      ezFileReader fileReader;

      if (fileReader.Open(szFileName, 1024 * 1024).Failed())
      {
        ezLog::Error("Couldn't open %s for BSP import.", szFileName);
        return nullptr;
      }

      ezUInt8 Temp[16384];

      while (ezUInt64 uiRead = fileReader.ReadBytes(Temp, 16384))
      {
        fileContent.PushBackRange(ezArrayPtr<ezUInt8>(Temp, (ezUInt32)uiRead));
      }
    }

    SourceBSP::File bspFile(fileContent);

    if (!bspFile.m_valid)
    {
      ezLog::Error("BSP header not valid for bsp file %s.", szFileName);
      return nullptr;
    }


    // Import the complete BSP geometry as a single mesh
    ezSharedPtr<Scene> outScene = EZ_DEFAULT_NEW(Scene);
    ezUniquePtr<Mesh> mesh(EZ_DEFAULT_NEW(Mesh));
    mesh->m_Name = "BSP Geometry";

    mesh->AddDataStream(ezGALVertexAttributeSemantic::Position, 3, VertexElementType::FLOAT);
    mesh->AddDataStream(ezGALVertexAttributeSemantic::Normal, 3, VertexElementType::FLOAT);
    mesh->AddDataStream(ezGALVertexAttributeSemantic::TexCoord0, 2, VertexElementType::FLOAT);

    if (SourceBSP::ConvertBSPGeometryToMesh(bspFile, mesh.Borrow(), outScene.Borrow()).Failed())
    {
      ezLog::Error("Couldn't convert BSP geometry to mesh for %s.", szFileName);
      return nullptr;
    }

    // Merge sub meshes with the same materials as the BSP splits would otherwise create enourmously
    // many draw calls.
    mesh->MergeSubMeshesWithSameMaterials();

    mesh->ComputeTangents();

    outScene->AddMesh(std::move(mesh));

    return outScene;
  }
}
