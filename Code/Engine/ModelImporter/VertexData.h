#pragma once

#include <ModelImporter/VertexIndex.h>

namespace ezModelImporter
{
  /// Index of a data entry in a VertexDataStream.
  ///
  /// This is just a fancy byte offset.
  struct EZ_MODELIMPORTER_DLL VertexDataIndex
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
  class EZ_MODELIMPORTER_DLL VertexDataStream
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(VertexDataStream);

  public:
    ~VertexDataStream() {}

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
    VertexElementType GetElementType() const        { return m_ElementType; }

    /// Returns size of a single element (not the vertex attribute).
    ///
    /// All element types are 4 bytes right now, but we use this function for future proving.
    ezUInt32 GetElementSize() const           { return 4; }

    /// Total size of a vertex attribute.
    ///
    /// This is also the minimal offset between two VertexDataIndex.
    ezUInt32 GetAttributeSize() const         { return GetElementSize() * m_uiNumElementsPerVertex; }

  private:
    friend class Mesh;
    template<typename Attribute, bool>
    friend class TypedVertexDataStreamView;

    VertexDataStream(ezUInt32 uiNumElementsPerVertex, ezUInt32 uiNumTriangles, VertexElementType elementType);

    // Note that the amount of triangles and vertices can only grow with this system.
    // If vertices are removed, they leave holes until they've been compacted.
    // Vertices leave only holes in m_Data if they had a data entry which was not referenced by any other vertex.

    /// Maps a VertexIndex to the first relevant float in m_Data.
    /// Mesh ensures that there are always enough entries in this array.
    ezDynamicArray<VertexDataIndex> m_IndexToData;

    /// First m_uiNumElementsPerVertex * GetElementTypeSize() elements are always zero, so that new vertices are simply default initialized.
    ezDynamicArray<char> m_Data;

    ezUInt32 m_uiNumElementsPerVertex;
    VertexElementType m_ElementType;
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
}

#include <ModelImporter/VertexData.inl>
