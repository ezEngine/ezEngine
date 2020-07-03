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
    ezFileReader fileReader;
    if (fileReader.Open(path.c_str()).Failed())
    {
      return 0;
    }

    ezUniquePtr<OpenFile> pOpenFile = EZ_DEFAULT_NEW(OpenFile);
    pOpenFile->m_Storage.ReadAll(fileReader);
    pOpenFile->m_Reader.SetStorage(&pOpenFile->m_Storage);

    return m_OpenFiles.Insert(std::move(pOpenFile)).ToRml();
  }

  void FileInterface::Close(Rml::Core::FileHandle file)
  {
    EZ_VERIFY(m_OpenFiles.Remove(FileId::FromRml(file)), "Invalid file handle {}", file);
  }

  size_t FileInterface::Read(void* buffer, size_t size, Rml::Core::FileHandle file)
  {
    auto& pOpenFile = m_OpenFiles[FileId::FromRml(file)];

    return pOpenFile->m_Reader.ReadBytes(buffer, size);
  }

  bool FileInterface::Seek(Rml::Core::FileHandle file, long offset, int origin)
  {
    auto& pOpenFile = m_OpenFiles[FileId::FromRml(file)];

    int iNewReadPosition = 0;
    int iEndPosition = pOpenFile->m_Reader.GetByteCount();

    if (origin == SEEK_SET)
    {
      iNewReadPosition = offset;
    }
    else if (origin == SEEK_CUR)
    {
      iNewReadPosition = pOpenFile->m_Reader.GetReadPosition() + offset;
    }
    else if (origin == SEEK_END)
    {
      iNewReadPosition = iEndPosition + offset;
    }

    if (iNewReadPosition >= 0 && iNewReadPosition <= iEndPosition)
    {
      pOpenFile->m_Reader.SetReadPosition(iNewReadPosition);
      return true;
    }

    EZ_ASSERT_NOT_IMPLEMENTED;
    return false;
  }

  size_t FileInterface::Tell(Rml::Core::FileHandle file)
  {
    auto& pOpenFile = m_OpenFiles[FileId::FromRml(file)];

    return pOpenFile->m_Reader.GetReadPosition();
  }

  size_t FileInterface::Length(Rml::Core::FileHandle file)
  {
    auto& pOpenFile = m_OpenFiles[FileId::FromRml(file)];

    return pOpenFile->m_Reader.GetByteCount();
  }

} // namespace ezRmlUiInternal
