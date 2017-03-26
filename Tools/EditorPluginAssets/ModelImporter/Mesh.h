#pragma once

#include <EditorPluginAssets/ModelImporter/Node.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/UniquePtr.h>

namespace ezModelImporter
{
  /// Range in a mesh using a specific material.
  struct SubMesh
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiFirstTriangle;
    ezUInt32 m_uiTriangleCount;

    MaterialHandle m_Material;
  };

  /// Index of a vertex in a triangle.
  ///
  /// This is *not* a index as in GPU index buffer.
  /// It can be used with different VertexDataStream to be resolved to a DataIndex.
  struct VertexIndex
  {
    EZ_DECLARE_POD_TYPE();
    bool operator == (VertexIndex b) { return m_Value == b.m_Value; }

  private:
    friend class Mesh;
    friend class VertexDataStream;
    template<typename Attribute, bool>
    friend class TypedVertexDataStreamView;

    EZ_ALWAYS_INLINE operator ezUInt32 () const { return m_Value; }

    ezUInt32 m_Value;
  };

  /// Index of a data entry in a VertexDataStream.
  ///
  /// This is just a fancy byte offset.
  struct VertexDataIndex
  {
    EZ_DECLARE_POD_TYPE();

    VertexDataIndex(ezUInt32 index) : m_Value(index) {}
    VertexDataIndex() : m_Value(s_InvalidDataIndex) {}
    void operator = (ezUInt32 index) { m_Value = index; }

    bool IsValid() const { return m_Value != s_InvalidDataIndex; }
    bool operator == (VertexDataIndex b) const { return m_Value == b.m_Value; }
    bool operator != (VertexDataIndex b) const { return m_Value != b.m_Value; }

    ezUInt32 GetValue() const { return m_Value; }

  private:
    friend class VertexDataStream;
    template<typename Attribute, bool>
    friend class TypedVertexDataStreamView;

    operator ezUInt32 () const { return m_Value; }

    static const ezUInt32 s_InvalidDataIndex = 0xFFFFFFFF;
    ezUInt32 m_Value;
  };

  /// Data storage for a vertex attribute.
  class VertexDataStream
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(VertexDataStream);

  public:
    ~VertexDataStream() {}

    /// Possible element types a vertex attribute consists of.
    enum class ElementType
    {
      FLOAT,
      INT32,
      UINT32,
    };


    /// Whether the given vertex has an entry in this data stream.
    bool HasValue(VertexIndex index) const;

    /// Retrieves the data index a vertex is pointing to.
    /// Index is not allowed to be invalid.
    VertexDataIndex GetDataIndex(VertexIndex index) const;

    /// Makes a vertex point to given data index.
    /// Neither index is allowed to be invalid.
    void SetDataIndex(VertexIndex vertex, VertexDataIndex data);


    /// Returns true if two given indices point at the same data.
    bool IsSharing(const VertexIndex& a, const VertexIndex& b) const;

    /// Reserves data memory for n value entries.
    void ReserveData(ezUInt32 numExpectedValues);

    /// Adds float values directly to the data array. Number of values needs to be a multiple of GetAttributeSize().
    /// Note that unless SetDataIndex is called, no vertex will point to this new data.
    void AddValues(const ezArrayPtr<char>& values);


    /// Number of elements per vertex attribute.
    ezUInt32 GetNumElementsPerVertex() const  { return m_uiNumElementsPerVertex; }

    /// Type of vertex element the vertex attribute consists of.
    ElementType GetElementType() const        { return m_ElementType; }

    /// Returns size of a single element (not the vertex attribute).
    ///
    /// All element types are 4 bytes right now, but we use this function for future proving.
    ezUInt32 GetElementSize() const           { return 4; }

    /// Total size of a vertex attribute.
    ///
    /// This is also the minimal offset between two VertexDataIndex.
    ezUInt32 GetAttributeSize() const         { return GetElementSize() * m_uiNumElementsPerVertex; }

  private:
    friend Mesh;
    template<typename Attribute, bool>
    friend class TypedVertexDataStreamView;

    VertexDataStream(ezUInt32 uiNumElementsPerVertex, ezUInt32 uiNumTriangles, ElementType elementType);

    // Note that the amount of triangles and vertices can only grow with this system.
    // If vertices are removed, they leave holes until they've been compacted.
    // Vertices leave only holes in m_Data if they had a data entry which was not referenced by any other vertex.

    /// Maps a VertexIndex to the first relevant float in m_Data.
    /// Mesh ensures that there are always enough entries in this array.
    ezDynamicArray<VertexDataIndex> m_IndexToData;

    /// First m_uiNumElementsPerVertex * GetElementTypeSize() elements are always zero, so that new vertices are simply default initialized.
    ezDynamicArray<char> m_Data;

    ezUInt32 m_uiNumElementsPerVertex;
    ElementType m_ElementType;
  };

  /// Vertex data stream wrapper that can do element specific queries and manipulation.
  ///
  /// Can only be created from a VertexDataStream.
  /// Creation guarantees that the element type is correct (therefore asserting only once for element count and element type).
  ///
  /// Implementation notes:
  /// Inheriting from VertexDataStream would be nice, but creation as such is not practical. Casting into it anyway would be possible but non-standard and weird.
  template<typename Attribute, bool ReadOnly = true>
  class TypedVertexDataStreamView
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(TypedVertexDataStreamView);

  public:
    using StreamType = std::conditional_t<ReadOnly, const VertexDataStream, VertexDataStream>;

    /// Asserts if the Attribute type is invalid or does not fulfill the element type and count properties of this data stream.
    TypedVertexDataStreamView(StreamType& sourceDataStream);

    /// Retrieves the value for a given VertexIndex.
    ///
    /// If index is invalid, a null attribute will be returned.
    Attribute GetValue(VertexIndex index) const;

    /// Retrieves the value for a given DataIndex.
    ///
    /// This requires one lookup less than using the VertexIndex.
    /// Data indices are local to a vertex stream!
    /// If index is invalid, a null attribute will be returned.
    Attribute GetValue(VertexDataIndex index) const;

    /// Sets a value for vertex. If the vertex is pointing to an invalid DataIndex, a new data entry will be created.
    std::enable_if_t<!ReadOnly> SetValue(VertexIndex index, const Attribute& value);

    /// Adds a vertex attribute value to the data array.
    std::enable_if_t<!ReadOnly> AddValue(const Attribute& value);

    /// Adds a vertex attribute values to the data array.
    std::enable_if_t<!ReadOnly> AddValues(const ezArrayPtr<Attribute>& values);

    /// Access to wrapped data stream.
    StreamType* operator -> () { return &m_DataStream; }

  private:
    StreamType& m_DataStream;
  };

  /// Triangle mesh imported from a raw asset file.
  ///
  /// Note that as of now we do not support other mesh representations, which means that any other topology needs to be triangulated on import.
  class Mesh : public HierarchyObject
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(Mesh);

  public:

    Mesh();
    Mesh(Mesh&& mesh);

    ~Mesh();

    // Triangles
  public:

    struct Triangle
    {
      EZ_DECLARE_POD_TYPE();

      void operator = (const Triangle& rhs);
      VertexIndex m_Vertices[3];
    };

    /// Adds a given number of triangles to the mesh.
    /// All vertex data points to zero elements by default.
    void AddTriangles(ezUInt32 num);
    ezUInt32 GetNumTriangles() const { return m_Triangles.GetCount(); }
    ezArrayPtr<const Triangle> GetTriangles() const { return ezMakeArrayPtr(m_Triangles); }
    ezArrayPtr<Triangle> GetTriangles()             { return ezMakeArrayPtr(m_Triangles); }

    // Data Streams
  public:

    /// Adds a new data stream with the given semantic.
    ///
    /// If a data stream already exists it will be returned, unless the existing data stream has a different element type or element count per vertex attribute.
    /// In this case nullptr will be returned.
    VertexDataStream* AddDataStream(ezGALVertexAttributeSemantic::Enum semantic, ezUInt32 uiNumElementsPerVertex, VertexDataStream::ElementType elementType = VertexDataStream::ElementType::FLOAT);

    /// Removes a data stream from the mesh.
    void RemoveDataStream(ezGALVertexAttributeSemantic::Enum semantic);

    /// Retrieves a data stream for a given semantic. Null if there is none.
    VertexDataStream* GetDataStream(ezGALVertexAttributeSemantic::Enum semantic);
    const VertexDataStream* GetDataStream(ezGALVertexAttributeSemantic::Enum semantic) const;

    // Submeshes
  public:

    ezUInt32 GetNumSubMeshes() const;
    const SubMesh& GetSubMesh(ezUInt32 idx) const;
    SubMesh& GetSubMesh(ezUInt32 idx);
    void AddSubMesh(SubMesh& mesh);

    // Processing
  public:

    /// Applies a transform to all directional and positional vertex data.
    void ApplyTransform(const ezTransform& transform);

    /// Adds all triangles, vertices and submeshes from an existing mesh.
    void AddData(const Mesh& mesh, const ezTransform& transform = ezTransform::Identity());

    /// Merges all sub-meshes that use the same material.
    /// Especially useful after merging meshes.
    void MergeSubMeshesWithSameMaterials();

    /// Computes vertex normals from position data.
    /// If the mesh already has a vertex stream for normals, they will be recomputed.
    /// Fails if there is no position stream.
    ezResult ComputeNormals();

    /// Computes vertex tangents and bitangent signs.
    ///
    /// Bitangent vertex stream will only have a single element which determines the sign of the bitangent. BiTangent = (normal x tangent) * sign
    /// If the mesh already has a vertex stream for (bi)tangent, they will be recomputed.
    /// Fails if there is no normal stream or there is no Texcoord0 stream with 2 components.
    ezResult ComputeTangents();

  private:

    ezDynamicArray<Triangle> m_Triangles;
    ezUInt32 m_uiNextUnusedVertexIndex;

    ezHashTable<ezInt32, VertexDataStream*> m_VertexDataStreams;

    /// List of submeshes. There should be always at least one submesh.
    ezDynamicArray<SubMesh> m_SubMeshes;
  };
}

#include <EditorPluginAssets/ModelImporter/Mesh.inl>
