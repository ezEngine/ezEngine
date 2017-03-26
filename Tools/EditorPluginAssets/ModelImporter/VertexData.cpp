#include <PCH.h>
#include <EditorPluginAssets/ModelImporter/VertexData.h>

namespace ezModelImporter
{
  VertexDataStream::VertexDataStream(ezUInt32 uiNumElementsPerVertex, ezUInt32 uiNumTriangles, VertexElementType elementType)
    : m_uiNumElementsPerVertex(uiNumElementsPerVertex)
    , m_ElementType(elementType)
  {
    m_IndexToData.SetCount(uiNumTriangles * 3);
  }

  void VertexDataStream::ReserveData(ezUInt32 numExpectedValues)
  {
    // +1 for the zero entry at the start of the array.
    m_Data.Reserve((numExpectedValues + 1) * GetAttributeSize());
  }
}
