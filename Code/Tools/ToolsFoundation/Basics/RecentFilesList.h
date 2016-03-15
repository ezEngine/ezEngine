#include <ToolsFoundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>

/// \brief Maintains a list of recently used files.
class EZ_TOOLSFOUNDATION_DLL ezRecentFilesList
{
public:
  ezRecentFilesList(ezUInt32 uiMaxElements) { m_uiMaxElements = uiMaxElements; }

  /// \brief Moves the inserted file to the front.
  void Insert(const char* szFile);

  /// \brief Returns all files in the list.
  const ezDeque<ezString>& GetFileList() const { return m_Files; }

  /// \brief Clears the list
  void Clear() { m_Files.Clear(); }

  /// \brief Saves the recent files list to the given file. Uses a simple text file format (one line per item).
  void Save(const char* szFile);

  /// \brief Loads the recent files list from the given file. Uses a simple text file format (one line per item).
  void Load(const char* szFile);

private:
  ezUInt32 m_uiMaxElements;
  ezDeque<ezString> m_Files;
};
