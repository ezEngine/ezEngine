#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>

#include <RmlUiPlugin/Implementation/FileInterface.h>

namespace ezRmlUiInternal
{
  FileInterface::FileInterface() = default;

  FileInterface::~FileInterface()
  {
    EZ_ASSERT_DEV(m_OpenFiles.IsEmpty(), "FileInterface has still open files");
  }

  Rml::FileHandle FileInterface::Open(const Rml::String& sPath)
  {
    ezFileReader fileReader;
    if (fileReader.Open(sPath.c_str()).Failed())
    {
      return 0;
    }

    ezUniquePtr<OpenFile> pOpenFile = EZ_DEFAULT_NEW(OpenFile);
    pOpenFile->m_Storage.ReadAll(fileReader);
    pOpenFile->m_Reader.SetStorage(&pOpenFile->m_Storage);

    return m_OpenFiles.Insert(std::move(pOpenFile)).ToRml();
  }

  void FileInterface::Close(Rml::FileHandle hFile)
  {
    EZ_VERIFY(m_OpenFiles.Remove(FileId::FromRml(hFile)), "Invalid file handle {}", hFile);
  }

  size_t FileInterface::Read(void* pBuffer, size_t uiSize, Rml::FileHandle hFile)
  {
    auto& pOpenFile = m_OpenFiles[FileId::FromRml(hFile)];

    return static_cast<size_t>(pOpenFile->m_Reader.ReadBytes(pBuffer, uiSize));
  }

  bool FileInterface::Seek(Rml::FileHandle hFile, long iOffset, int iOrigin)
  {
    auto& pOpenFile = m_OpenFiles[FileId::FromRml(hFile)];

    int iNewReadPosition = 0;
    int iEndPosition = static_cast<int>(pOpenFile->m_Reader.GetByteCount32());

    if (iOrigin == SEEK_SET)
    {
      iNewReadPosition = iOffset;
    }
    else if (iOrigin == SEEK_CUR)
    {
      iNewReadPosition = static_cast<int>(pOpenFile->m_Reader.GetReadPosition() + iOffset);
    }
    else if (iOrigin == SEEK_END)
    {
      iNewReadPosition = iEndPosition + iOffset;
    }

    if (iNewReadPosition >= 0 && iNewReadPosition <= iEndPosition)
    {
      pOpenFile->m_Reader.SetReadPosition(iNewReadPosition);
      return true;
    }

    EZ_ASSERT_NOT_IMPLEMENTED;
    return false;
  }

  size_t FileInterface::Tell(Rml::FileHandle hFile)
  {
    auto& pOpenFile = m_OpenFiles[FileId::FromRml(hFile)];

    return pOpenFile->m_Reader.GetReadPosition();
  }

  size_t FileInterface::Length(Rml::FileHandle hFile)
  {
    auto& pOpenFile = m_OpenFiles[FileId::FromRml(hFile)];

    return pOpenFile->m_Reader.GetByteCount64();
  }

} // namespace ezRmlUiInternal
