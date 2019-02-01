namespace ezModelImporter
{
  inline void Mesh::Triangle::operator = (const Mesh::Triangle& rhs)
  {
    m_Vertices[0] = rhs.m_Vertices[0];
    m_Vertices[1] = rhs.m_Vertices[1];
    m_Vertices[2] = rhs.m_Vertices[2];
  }


  template<int NumStreams>
  inline bool Mesh::DataIndexBundle<NumStreams>::operator == (const DataIndexBundle& dataIndex) const
  {
    for (int i = 0; i < NumStreams; ++i)
    {
      if (m_indices[i] != dataIndex.m_indices[i])
        return false;
    }
    return true;
  }

  template<int NumStreams>
  inline const ezModelImporter::VertexDataIndex Mesh::DataIndexBundle<NumStreams>::operator [] (int i) const
  {
    return m_indices[i];
  }

  template<int NumStreams>
  inline ezModelImporter::VertexDataIndex& Mesh::DataIndexBundle<NumStreams>::operator [] (int i)
  {
    return m_indices[i];
  }

  template<int NumStreams>
  inline ezResult Mesh::GenerateInterleavedVertexMapping(const ezGALVertexAttributeSemantic::Enum (&dataStreamSemantics)[NumStreams],
                                                         ezHashTable<Mesh::DataIndexBundle<NumStreams>, ezUInt32>& outDataIndices_to_InterleavedVertexIndices, ezDynamicArray<ezUInt32>& outTriangleVertexIndices) const
  {
    const VertexDataStream* dataStreams[NumStreams];
    for (int i = 0; i < NumStreams; ++i)
    {
      dataStreams[i] = this->GetDataStream(dataStreamSemantics[i]);
      if (!dataStreams[i])
        return EZ_FAILURE;
    }
    GenerateInterleavedVertexMapping(ezMakeArrayPtr(this->m_Triangles), dataStreams, outDataIndices_to_InterleavedVertexIndices, outTriangleVertexIndices);
    return EZ_SUCCESS;
  }

  template<int NumStreams>
  inline void Mesh::GenerateInterleavedVertexMapping(const ezArrayPtr<const Triangle>& triangles, const VertexDataStream* (&dataStreams)[NumStreams],
                                                     ezHashTable<Mesh::DataIndexBundle<NumStreams>, ezUInt32>& outDataIndices_to_InterleavedVertexIndices, ezDynamicArray<ezUInt32>& outTriangleVertexIndices)
  {
    outTriangleVertexIndices.SetCountUninitialized(triangles.GetCount() * 3);

    ezUInt32 nextVertexIndex = 0;
    DataIndexBundle<NumStreams> dataIndices;
    for (ezUInt32 t = 0; t < triangles.GetCount(); ++t)
    {
      for (int v = 0; v < 3; ++v)
      {
        for (int stream = 0; stream < NumStreams; ++stream)
        {
          dataIndices[stream] = dataStreams[stream] ? dataStreams[stream]->GetDataIndex(triangles[t].m_Vertices[v]) : VertexDataIndex(0);
        }

        ezUInt32 gpuVertexIndex = nextVertexIndex;
        if (outDataIndices_to_InterleavedVertexIndices.TryGetValue(dataIndices, gpuVertexIndex) == false)
        {
          outDataIndices_to_InterleavedVertexIndices.Insert(dataIndices, nextVertexIndex);
          ++nextVertexIndex;
        }

        outTriangleVertexIndices[t * 3 + v] = gpuVertexIndex;
      }
    }
  }
}

template <int NumStreams>
struct ezHashHelper<typename ezModelImporter::Mesh::DataIndexBundle<NumStreams>>
{
  typedef ezModelImporter::Mesh::DataIndexBundle<NumStreams> ValueType;

  static ezUInt32 Hash(const ValueType& value)
  {
    return ezHashingUtils::xxHash32(&value, sizeof(ValueType));
  }

  static bool Equal(const ValueType& a, const ValueType& b)
  {
    return a == b;
  }
};
