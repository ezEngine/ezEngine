#include <RmlUiPluginDLL.h>

#include <Foundation/IO/FileSystem/FileReader.h>

#include <RmlUiPlugin/Implementation/FileInterface.h>

namespace ezRmlUiInternal
{
  FileInterface::FileInterface() = default;

  FileInterface::~FileInterface()
  {
    EZ_ASSERT_DEV(m_OpenFiles.IsEmpty(), "FileInterface has still open files");
  }

  Rml::Core::FileHandle FileInterface::Open(const Rml::Core::String& path)
  {
    ezUniquePtr<ezFileReader> pFileReader = EZ_DEFAULT_NEW(ezFileReader);
    if (pFileReader->Open(path.c_str()).Failed())
    {
      return 0;
    }

    m_OpenFiles.Insert(m_NextFileHandle, std::move(pFileReader));

    return m_NextFileHandle++;
  }

  void FileInterface::Close(Rml::Core::FileHandle file)
  {
    EZ_VERIFY(m_OpenFiles.Remove(file), "Invalid file handle {}", file);
  }

  size_t FileInterface::Read(void* buffer, size_t size, Rml::Core::FileHandle file)
  {
    auto it = m_OpenFiles.Find(file);
    EZ_ASSERT_DEV(it.IsValid(), "Invalid file handle {}", file);

    return it.Value()->ReadBytes(buffer, size);
  }

  bool FileInterface::Seek(Rml::Core::FileHandle file, long offset, int origin)
  {
    auto it = m_OpenFiles.Find(file);
    EZ_ASSERT_DEV(it.IsValid(), "Invalid file handle {}", file);

    if (origin == SEEK_CUR && offset >= 0)
    {
      ezUInt64 skippedBytes = it.Value()->SkipBytes(offset);
      return skippedBytes == offset;
    }

    EZ_ASSERT_NOT_IMPLEMENTED;
    return false;
  }

  size_t FileInterface::Tell(Rml::Core::FileHandle file)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
    return 0;
  }

  size_t FileInterface::Length(Rml::Core::FileHandle file)
  {
    auto it = m_OpenFiles.Find(file);
    EZ_ASSERT_DEV(it.IsValid(), "Invalid file handle {}", file);

    return it.Value()->GetFileSize();
  }

} // namespace ezRmlUiInternal
