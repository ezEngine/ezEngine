#pragma once

#include <ModelImporter/Plugin.h>

namespace ezModelImporter
{
  /// Index of a vertex in a triangle.
  ///
  /// This is *not* a index as in GPU index buffer.
  /// It can be used with different VertexDataStream to be resolved to a DataIndex.
  struct EZ_MODELIMPORTER_DLL VertexIndex
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

  /// Possible element types a vertex attribute consists of.
  enum class VertexElementType
  {
    FLOAT,
    INT32,
    UINT32,
  };
}
