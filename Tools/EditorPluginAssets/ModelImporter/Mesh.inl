#pragma once

#include <EditorPluginAssets/ModelImporter/Node.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include "Mesh.h"

namespace ezModelImporter
{
  inline bool VertexDataStream::HasValue(VertexIndex index) const
  {
    return m_IndexToData.GetCount() > index && m_IndexToData[index].IsValid();
  }

  inline VertexDataIndex VertexDataStream::GetDataIndex(VertexIndex index) const
  {
    EZ_ASSERT_DEBUG(index < m_IndexToData.GetCount(), "Vertex index is not mapped. Mesh should have made sure that there are enough indices!");
    return m_IndexToData[index];
  }

  inline void VertexDataStream::SetDataIndex(VertexIndex vertex, VertexDataIndex data)
  {
    EZ_ASSERT_DEBUG(data.IsValid(), "Vertex data index is invalid!");
    EZ_ASSERT_DEBUG(data.m_Value % m_uiNumElementsPerVertex == 0, "Vertex data index value must be a multiple of the element count per vertex ({0} with this instance).", m_uiNumElementsPerVertex);
    EZ_ASSERT_DEBUG(vertex < m_IndexToData.GetCount(), "Vertex index is not mapped. Mesh should have made sure that there are enough indices!");

    m_IndexToData[vertex] = data;
  }

  inline float VertexDataStream::GetValueFloat(VertexIndex index) const
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 1, "Data stream has {0} values, not 1", m_uiNumElementsPerVertex);
    if (!HasValue(index)) return 0.0f;
    return m_Data[m_IndexToData[index]];
  }

  inline const ezVec2 VertexDataStream::GetValueVec2(VertexIndex index) const
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 2, "Data stream has {0} values, not 2", m_uiNumElementsPerVertex);
    if (!HasValue(index)) return ezVec2(0.0f, 0.0f);
    return *reinterpret_cast<const ezVec2*>(&m_Data[m_IndexToData[index]]);
  }

  inline const ezVec3 VertexDataStream::GetValueVec3(VertexIndex index) const
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 3, "Data stream has {0} values, not 3", m_uiNumElementsPerVertex);
    if (!HasValue(index)) return ezVec3(0.0f, 0.0f, 0.0f);
    return *reinterpret_cast<const ezVec3*>(&m_Data[m_IndexToData[index]]);
  }

  inline const ezVec4 VertexDataStream::GetValueVec4(VertexIndex index) const
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 4, "Data stream has {0} values, not 4", m_uiNumElementsPerVertex);
    if (!HasValue(index)) return ezVec4(0.0f, 0.0f, 0.0f, 0.0f);
    return *reinterpret_cast<const ezVec4*>(&m_Data[m_IndexToData[index]]);
  }

  inline float VertexDataStream::GetValueFloat(VertexDataIndex index) const
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 1, "Data stream has {0} values, not 1", m_uiNumElementsPerVertex);
    if (!index.IsValid()) return 0.0f;
    return m_Data[index];
  }

  inline const ezVec2 VertexDataStream::GetValueVec2(VertexDataIndex index) const
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 2, "Data stream has {0} values, not 2", m_uiNumElementsPerVertex);
    if (!index.IsValid()) return ezVec2(0.0f, 0.0f);
    return *reinterpret_cast<const ezVec2*>(&m_Data[index]);
  }

  inline const ezVec3 VertexDataStream::GetValueVec3(VertexDataIndex index) const
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 3, "Data stream has {0} values, not 3", m_uiNumElementsPerVertex);
    if (!index.IsValid()) return ezVec3(0.0f, 0.0f, 0.0f);
    return *reinterpret_cast<const ezVec3*>(&m_Data[index]);
  }

  inline const ezVec4 VertexDataStream::GetValueVec4(VertexDataIndex index) const
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 4, "Data stream has {0} values, not 4", m_uiNumElementsPerVertex);
    if (!index.IsValid()) return ezVec4(0.0f, 0.0f, 0.0f, 0.0f);
    return *reinterpret_cast<const ezVec4*>(&m_Data[index]);
  }


  inline void VertexDataStream::SetValue(VertexIndex index, float value)
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 1, "Data stream has {0} values, not 1", m_uiNumElementsPerVertex);
    if (!HasValue(index))
    {
      m_IndexToData[index] = m_Data.GetCount();
      m_Data.PushBack(value);
    }
    else
      m_Data[m_IndexToData[index]] = value;
  }

  inline void VertexDataStream::SetValue(VertexIndex index, const ezVec2& value)
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 2, "Data stream has {0} values, not 2", m_uiNumElementsPerVertex);
    if (!HasValue(index))
    {
      m_IndexToData[index] = m_Data.GetCount();
      m_Data.Reserve(m_Data.GetCount() + 2);
      m_Data.PushBackUnchecked(value.x);
      m_Data.PushBackUnchecked(value.y);
    }
    else
      *reinterpret_cast<ezVec2*>(&m_Data[m_IndexToData[index]]) = value;
  }

  inline void VertexDataStream::SetValue(VertexIndex index, const ezVec3& value)
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 3, "Data stream has {0} values, not 3", m_uiNumElementsPerVertex);
    if (!HasValue(index))
    {
      m_IndexToData[index] = m_Data.GetCount();
      m_Data.Reserve(m_Data.GetCount() + 3);
      m_Data.PushBackUnchecked(value.x);
      m_Data.PushBackUnchecked(value.y);
      m_Data.PushBackUnchecked(value.z);
    }
    else
      *reinterpret_cast<ezVec3*>(&m_Data[m_IndexToData[index]]) = value;
  }

  inline void VertexDataStream::SetValue(VertexIndex index, const ezVec4& value)
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 4, "Data stream has {0} values, not 4", m_uiNumElementsPerVertex);
    if (!HasValue(index))
    {
      m_IndexToData[index] = m_Data.GetCount();
      m_Data.Reserve(m_Data.GetCount() + 4);
      m_Data.PushBackUnchecked(value.x);
      m_Data.PushBackUnchecked(value.y);
      m_Data.PushBackUnchecked(value.z);
      m_Data.PushBackUnchecked(value.w);
    }
    else
      *reinterpret_cast<ezVec4*>(&m_Data[m_IndexToData[index]]) = value;
  }

  //inline void VertexDataStream::ShareData(VertexIndex from, VertexIndex pointTo)
  //{
  //  if (!HasValue(pointTo))
  //  {
  //    m_IndexToData[pointTo] = m_Data.GetCount();
  //    m_Data.Reserve(m_Data.GetCount() + m_uiNumElementsPerVertex);
  //    for (ezUInt32 i = 0; i < m_uiNumElementsPerVertex; ++i)
  //      m_Data.PushBackUnchecked(0.0f);
  //  }
  //  m_IndexToData[from] = m_IndexToData[pointTo];
  //}

  inline bool VertexDataStream::IsSharing(const VertexIndex& a, const VertexIndex& b) const
  {
    EZ_ASSERT_DEBUG(a < m_IndexToData.GetCount() && b < m_IndexToData.GetCount(), "Vertex index is not mapped. Mesh should have made sure that there are enough indices!");
    return m_IndexToData[a] == m_IndexToData[b];
  }

  inline void VertexDataStream::AddValue(float value)
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 1, "Data stream has {0} elements per vertex, not 1", m_uiNumElementsPerVertex);
    m_Data.PushBack(value);
  }
  inline void VertexDataStream::AddValue(const ezVec2& value)
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 2, "Data stream has {0} elements per vertex, not 2", m_uiNumElementsPerVertex);
    m_Data.Reserve(m_Data.GetCount() + 2);
    m_Data.PushBackUnchecked(value.x);
    m_Data.PushBackUnchecked(value.y);
  }
  inline void VertexDataStream::AddValue(const ezVec3& value)
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 3, "Data stream has {0} elements per vertex, not 3", m_uiNumElementsPerVertex);
    m_Data.Reserve(m_Data.GetCount() + 3);
    m_Data.PushBackUnchecked(value.x);
    m_Data.PushBackUnchecked(value.y);
    m_Data.PushBackUnchecked(value.z);
  }
  inline void VertexDataStream::AddValue(const ezVec4& value)
  {
    EZ_ASSERT_DEBUG(m_uiNumElementsPerVertex == 4, "Data stream has {0} elements per vertex, not 4", m_uiNumElementsPerVertex);
    m_Data.Reserve(m_Data.GetCount() + 4);
    m_Data.PushBackUnchecked(value.x);
    m_Data.PushBackUnchecked(value.y);
    m_Data.PushBackUnchecked(value.z);
    m_Data.PushBackUnchecked(value.w);
  }

  inline void VertexDataStream::AddValues(const ezArrayPtr<float>& values)
  {
    EZ_ASSERT_DEBUG(values.GetCount() % m_uiNumElementsPerVertex == 0, "Number of added values needs to be a multiple of {0}", m_uiNumElementsPerVertex);
    m_Data.PushBackRange(values);
  }

  inline void Mesh::Triangle::operator = (const Mesh::Triangle& rhs)
  {
    m_Vertices[0] = rhs.m_Vertices[0];
    m_Vertices[1] = rhs.m_Vertices[1];
    m_Vertices[2] = rhs.m_Vertices[2];
  }
}
