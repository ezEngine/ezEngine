#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <ToolsFoundation/FileSystem/DataDirPath.h>

bool ezDataDirPath::UpdateDataDirInfos(ezArrayPtr<ezString> dataDirRoots, ezUInt32 uiLastKnownDataDirIndex /*= 0*/) const
{
  const ezUInt32 uiCount = dataDirRoots.GetCount();
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    ezUInt32 uiCurrentIndex = (uiLastKnownDataDirIndex + i) % uiCount;
    EZ_ASSERT_DEBUG(!dataDirRoots[uiCurrentIndex].EndsWith_NoCase("/"), "");
    if (m_sAbsolutePath.StartsWith_NoCase(dataDirRoots[uiCurrentIndex]) && !dataDirRoots[uiCurrentIndex].IsEmpty())
    {
      m_uiDataDirIndex = static_cast<ezUInt8>(uiCurrentIndex);
      const char* szParentFolder = ezPathUtils::FindPreviousSeparator(m_sAbsolutePath.GetData(), m_sAbsolutePath.GetData() + dataDirRoots[uiCurrentIndex].GetElementCount());
      m_uiDataDirParent = static_cast<ezUInt16>(szParentFolder - m_sAbsolutePath.GetData());
      m_uiDataDirLength = static_cast<ezUInt8>(dataDirRoots[uiCurrentIndex].GetElementCount() - m_uiDataDirParent);
      return true;
    }
  }

  m_uiDataDirParent = 0;
  m_uiDataDirLength = 0;
  m_uiDataDirIndex = 0;
  return false;
}
