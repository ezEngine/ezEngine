#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Vec3.h>

class ezWorld;

/// \brief A utility to gather raw geometry from a world
///
/// The utility sends ezMsgExtractGeometry to world components and they may fill out the geometry information.
/// \a ExtractionMode defines what the geometry is needed for. This ranges from finding geometry that is used to generate the navmesh from
/// to exporting the geometry to a file for use in another program, e.g. a modeling software.
class EZ_CORE_DLL ezWorldGeoExtractionUtil
{
public:
  struct Vertex
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vPosition;
    // ezVec2 m_vTexCoord;
  };

  struct Triangle
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiVertexIndices[3];
  };

  /// \brief Geometry can also be described as a number of shapes, which can be more efficient in some cases
  struct Shape
  {
    ezVec3 m_vPosition;
    ezQuat m_qRotation;

    // TODO ground type etc.
  };

  struct BoxShape : public Shape
  {
    ezVec3 m_vHalfExtents;
  };

  struct Geometry
  {
    ezDeque<Vertex> m_Vertices;
    ezDeque<Triangle> m_Triangles;
    ezDeque<BoxShape> m_BoxShapes;
  };

  /// \brief Describes what the geometry is needed for
  enum class ExtractionMode
  {
    RenderMesh,        ///< The render geometry is desired. Typically for exporting it to file.
    CollisionMesh,     ///< The collision geometry is desired. Typically for exporting it to file.
    NavMeshGeneration, ///< The geometry that participates in navmesh generation is desired.
  };

  /// \brief Extracts the desired geometry from all objects in a world
  ///
  /// The geometry object is not cleared, so this can be called repeatedly to append more data.
  static void ExtractWorldGeometry(Geometry& geo, const ezWorld& world, ExtractionMode mode);

  /// \brief Extracts the desired geometry from a specified subset of objects in a world
  ///
  /// The geometry object is not cleared, so this can be called repeatedly to append more data.
  static void ExtractWorldGeometry(Geometry& geo, const ezWorld& world, ExtractionMode mode, const ezDeque<ezGameObjectHandle>& selection);

  /// \brief Writes the given geometry in .obj format to file
  static void WriteWorldGeometryToOBJ(const char* szFile, const Geometry& geo);
};

/// \brief Sent by ezWorldGeoExtractionUtil to gather geometry information about objects in a world
///
/// The mode defines what the geometry is needed for, thus components should decide to participate or not
/// and how detailed the geometry is they return.
struct EZ_CORE_DLL ezMsgExtractGeometry : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgExtractGeometry, ezMessage);

  /// \brief Specifies what the geometry is extracted for, and thus what the message handler should write back
  ezWorldGeoExtractionUtil::ExtractionMode m_Mode = ezWorldGeoExtractionUtil::ExtractionMode::RenderMesh;

  /// \brief Append data to this to describe the requested world geometry
  ezWorldGeoExtractionUtil::Geometry* m_pWorldGeometry = nullptr;
};
