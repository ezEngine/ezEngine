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
    EZ_ASSERT_DEBUG(data.m_Value % GetAttributeSize() == 0, "Vertex data index value must be a multiple of the vertex attribute size ({0} in this instance).", GetAttributeSize());
    EZ_ASSERT_DEBUG(vertex < m_IndexToData.GetCount(), "Vertex index is not mapped. Mesh should have made sure that there are enough indices!");

    m_IndexToData[vertex] = data;
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

  inline void VertexDataStream::AddValues(const ezArrayPtr<char>& values)
  {
    EZ_ASSERT_DEBUG(values.GetCount() % GetAttributeSize() == 0, "Number of added values needs to be a multiple of {0}", GetAttributeSize());
    m_Data.PushBackRange(values);
  }


  template<typename Attribute, bool ReadOnly>
  TypedVertexDataStreamView<Attribute, ReadOnly>::TypedVertexDataStreamView(StreamType& sourceDataStream)
    : m_DataStream(sourceDataStream)
  {
    EZ_ASSERT_DEV(sizeof(Attribute) % this->m_DataStream.GetAttributeSize() == 0,
                  "Size of attribute type ({0}) must equal the attribute size of the wrapped vertex stream ({1})", (ezUInt32)sizeof(Attribute), this->m_DataStream.GetAttributeSize());

    // We could allow arbitrary types, but for sanity we don't. See static_assert in class decl.
    // This would be a great place for C++17 constexpr if.

    static_assert(
      std::is_same<float, Attribute>::value ||
      std::is_same<ezInt32, Attribute>::value ||
      std::is_same<ezUInt32, Attribute>::value ||
      std::is_same<ezVec2, Attribute>::value ||
      std::is_same<ezVec3, Attribute>::value ||
      std::is_same<ezVec4, Attribute>::value ||
      std::is_same<ezVec2I32, Attribute>::value ||
      std::is_same<ezVec3I32, Attribute>::value ||
      std::is_same<ezVec4I32, Attribute>::value ||
      std::is_same<ezVec2U32, Attribute>::value ||
      std::is_same<ezVec3U32, Attribute>::value ||
      std::is_same<ezVec4U32, Attribute>::value,
      "Invalid attribute type.");

#define STATIC_VEC_CHECK(ELEM_COUNT, TYPE, TYPENAME) \
    else if (std::is_same<ezVec##ELEM_COUNT##Template<TYPE>, Attribute>::value) \
    { \
      EZ_ASSERT_DEV(sourceDataStream.GetElementType() == VertexElementType::TYPENAME && \
                    sourceDataStream.GetNumElementsPerVertex() == ELEM_COUNT, "Vertex data stream is incompatible with the given vector type!"); \
    }

    if (std::is_same<float, Attribute>::value)
    {
      EZ_ASSERT_DEV(sourceDataStream.GetElementType() == VertexElementType::FLOAT &&
        sourceDataStream.GetNumElementsPerVertex() == 1, "Vertex data stream is incompatible with float attributes!");
    }
    else if (std::is_same<ezInt32, Attribute>::value)
    {
      EZ_ASSERT_DEV(sourceDataStream.GetElementType() == VertexElementType::INT32 &&
        sourceDataStream.GetNumElementsPerVertex() == 1, "Vertex data stream is incompatible with ezInt32 attributes!");
    }
    else if (std::is_same<ezUInt32, Attribute>::value)
    {
      EZ_ASSERT_DEV(sourceDataStream.GetElementType() == VertexElementType::UINT32 &&
        sourceDataStream.GetNumElementsPerVertex() == 1, "Vertex data stream is incompatible with ezUInt32 attributes!");
    }
    STATIC_VEC_CHECK(2, float, FLOAT)
    STATIC_VEC_CHECK(3, float, FLOAT)
    STATIC_VEC_CHECK(4, float, FLOAT)
    STATIC_VEC_CHECK(2, ezInt32, INT32)
    STATIC_VEC_CHECK(3, ezInt32, INT32)
    STATIC_VEC_CHECK(4, ezInt32, INT32)
    STATIC_VEC_CHECK(2, ezUInt32, UINT32)
    STATIC_VEC_CHECK(3, ezUInt32, UINT32)
    STATIC_VEC_CHECK(4, ezUInt32, UINT32)

#undef VEC_CHECK
  }

  template<typename Attribute, bool ReadOnly>
  inline Attribute TypedVertexDataStreamView<Attribute, ReadOnly>::GetValue(VertexIndex index) const
  {
    if (!this->m_DataStream.HasValue(index)) return Attribute { 0 };
    return *reinterpret_cast<const Attribute*>(&this->m_DataStream.m_Data[this->m_DataStream.m_IndexToData[index]]);
  }

  template<typename Attribute, bool ReadOnly>
  inline Attribute TypedVertexDataStreamView<Attribute, ReadOnly>::GetValue(VertexDataIndex index) const
  {
    if (!index.IsValid()) return Attribute { 0 };
    return *reinterpret_cast<const Attribute*>(&this->m_DataStream.m_Data[index]);
  }

  template<typename Attribute, bool ReadOnly>
  inline std::enable_if_t<!ReadOnly> TypedVertexDataStreamView<Attribute, ReadOnly>::SetValue(VertexIndex index, const Attribute& value)
  {
    const char* data = reinterpret_cast<const char*>(&value);

    if (!this->m_DataStream.HasValue(index))
    {
      this->m_DataStream.m_IndexToData[index] = this->m_DataStream.m_Data.GetCount();
      this->m_DataStream.m_Data.PushBackRange(ezMakeArrayPtr(data, sizeof(Attribute)));
    }
    else
    {
      // Not using ezMemoryUtils - we know what we're doing here, it should be just raw data and we did all the necessary checks upfront.
      memcpy(&this->m_DataStream.m_Data[this->m_DataStream.m_IndexToData[index]], data, sizeof(Attribute));
    }
  }

  template<typename Attribute, bool ReadOnly>
  inline std::enable_if_t<!ReadOnly> TypedVertexDataStreamView<Attribute, ReadOnly>::AddValue(const Attribute& value)
  {
    const char* data = reinterpret_cast<const char*>(&value);
    this->m_DataStream.m_Data.PushBackRange(ezMakeArrayPtr(data, sizeof(Attribute)));
  }

  template<typename Attribute, bool ReadOnly>
  inline std::enable_if_t<!ReadOnly> TypedVertexDataStreamView<Attribute, ReadOnly>::AddValues(const ezArrayPtr<Attribute>& values)
  {
    const char* data = reinterpret_cast<const char*>(values.GetPtr());
    this->m_DataStream.m_Data.PushBackRange(ezMakeArrayPtr(data, values.GetCount() * sizeof(Attribute)));
  }
}
