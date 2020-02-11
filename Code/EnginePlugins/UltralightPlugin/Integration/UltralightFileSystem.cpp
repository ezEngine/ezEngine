
#include <PCH.h>

#include <UltralightPlugin/Integration/UltralightFileSystem.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>

ezUltralightFileSystem::ezUltralightFileSystem() = default;
ezUltralightFileSystem::~ezUltralightFileSystem() = default;


bool ezUltralightFileSystem::FileExists(const ultralight::String16& path)
{
  return ezFileSystem::ExistsFile(ezStringUtf8(path.data()).GetData());
}

bool ezUltralightFileSystem::DeleteFile_(const ultralight::String16& path)
{
  return false;
}

bool ezUltralightFileSystem::DeleteEmptyDirectory(const ultralight::String16& path)
{
  return false;
}

bool ezUltralightFileSystem::MoveFile_(const ultralight::String16& old_path, const ultralight::String16& new_path)
{
  return false;
}

bool ezUltralightFileSystem::GetFileSize(const ultralight::String16& path, int64_t& result)
{
  ezFileStats stats;
  if (ezFileSystem::GetFileStats(ezStringUtf8(path.data()).GetData(), stats).Succeeded())
  {
    result = static_cast<int64_t>(stats.m_uiFileSize);
    return true;
  }

  result = 0;
  return false;
}

bool ezUltralightFileSystem::GetFileSize(ultralight::FileHandle handle, int64_t& result)
{
  ezFileReader* pReader = nullptr;

  {
    EZ_LOCK(m_FileReaderMutex);

    if (auto pValue = m_OpenFileReaders.GetValue(handle))
    {
      pReader = pValue->Borrow();
    }
  }

  if (pReader)
  {
    result = static_cast<int64_t>(pReader->GetFileSize());
    return true;
  }

  return false;
}

bool ezUltralightFileSystem::GetFileMimeType(const ultralight::String16& path, ultralight::String16& result)
{
  ezStringBuilder data = ezStringUtf8(path.data()).GetData();
  ezStringBuilder retVal;

  if (data.HasExtension(".htm") || data.HasExtension(".html"))
  {
    retVal = "text/html";
  }
  else if (data.HasExtension(".jpg") || data.HasExtension(".jpeg"))
  {
    retVal = "image/jpeg";
  }
  else if (data.HasExtension(".png"))
  {
    retVal = "image/png";
  }
  else if (data.HasExtension(".gif"))
  {
    retVal = "image/gif";
  }
  else if (data.HasExtension(".bmp"))
  {
    retVal = "image/bmp";
  }
  else if (data.HasExtension(".ico"))
  {
    retVal = "image/vnd.microsoft.icon";
  }
  else if (data.HasExtension(".svg"))
  {
    retVal = "image/svg+xml";
  }
  else if (data.HasExtension(".css"))
  {
    retVal = "text/css";
  }
  else if (data.HasExtension(".js"))
  {
    retVal = "text/javascript";
  }
  else if (data.HasExtension(".json"))
  {
    retVal = "application/json";
  }
  else if (data.HasExtension(".otf"))
  {
    retVal = "font/otf";
  }
  else if (data.HasExtension(".ttf"))
  {
    retVal = "font/ttf";
  }
  else if (data.HasExtension(".txt"))
  {
    retVal = "text/plain";
  }


  if (retVal.IsEmpty())
    return false;
  else
  {
    result = ultralight::String16(retVal);
    return true;
  }
}

bool ezUltralightFileSystem::GetFileModificationTime(const ultralight::String16& path, time_t& result)
{
  return false;
}

bool ezUltralightFileSystem::GetFileCreationTime(const ultralight::String16& path, time_t& result)
{
  return false;
}

ultralight::MetadataType ezUltralightFileSystem::GetMetadataType(const ultralight::String16& path)
{
  return ultralight::kMetadataType_Unknown;
}

ultralight::String16 ezUltralightFileSystem::GetPathByAppendingComponent(const ultralight::String16& path, const ultralight::String16& component)
{
  return "";
}

bool ezUltralightFileSystem::CreateDirectory_(const ultralight::String16& path)
{
  return false;
}

ultralight::String16 ezUltralightFileSystem::GetHomeDirectory()
{
  return "";
}

ultralight::String16 ezUltralightFileSystem::GetFilenameFromPath(const ultralight::String16& path)
{
  return "";
}

ultralight::String16 ezUltralightFileSystem::GetDirectoryNameFromPath(const ultralight::String16& path)
{
  return "";
}

bool ezUltralightFileSystem::GetVolumeFreeSpace(const ultralight::String16& path, uint64_t& result)
{
  return false;
}

int32_t ezUltralightFileSystem::GetVolumeId(const ultralight::String16& path)
{
  return 0;
}

ultralight::Ref<ultralight::String16Vector> ezUltralightFileSystem::ListDirectory(const ultralight::String16& path, const ultralight::String16& filter)
{
  ultralight::Ref<ultralight::String16Vector> entries = ultralight::String16Vector::Create();

  return entries;
}

ultralight::String16 ezUltralightFileSystem::OpenTemporaryFile(const ultralight::String16& prefix, ultralight::FileHandle& handle)
{
  return "";
}

ultralight::FileHandle ezUltralightFileSystem::OpenFile(const ultralight::String16& path, bool open_for_writing)
{
  EZ_ASSERT_DEV(!open_for_writing, "Write access is not implemented!");

  ezUniquePtr<ezFileReader> pReader = EZ_DEFAULT_NEW(ezFileReader);

  if (pReader->Open(ezStringUtf8(path.data()).GetData()).Succeeded())
  {
    EZ_LOCK(m_FileReaderMutex);

    auto fileHandle = m_NextFileHandle++;
    m_OpenFileReaders.Insert(fileHandle, std::move(pReader));

    return fileHandle;
  }

  return 0;
}

void ezUltralightFileSystem::CloseFile(ultralight::FileHandle& handle)
{
  EZ_LOCK(m_FileReaderMutex);

  m_OpenFileReaders.Remove(handle);
}

int64_t ezUltralightFileSystem::SeekFile(ultralight::FileHandle handle, int64_t offset, ultralight::FileSeekOrigin origin)
{
  return 0;
}

bool ezUltralightFileSystem::TruncateFile(ultralight::FileHandle handle, int64_t offset)
{
  return false;
}

int64_t ezUltralightFileSystem::WriteToFile(ultralight::FileHandle handle, const char* data, int64_t length)
{
  return 0;
}

int64_t ezUltralightFileSystem::ReadFromFile(ultralight::FileHandle handle, char* data, int64_t length)
{
  ezFileReader* pReader = nullptr;

  {
    EZ_LOCK(m_FileReaderMutex);

    if (auto pValue = m_OpenFileReaders.GetValue(handle))
    {
      pReader = pValue->Borrow();
    }
  }

  if (pReader)
  {
    return static_cast<int64_t>(pReader->ReadBytes(data, static_cast<ezUInt64>(length)));
  }
  else
  {
    return 0;
  }
}

bool ezUltralightFileSystem::CopyFile_(const ultralight::String16& source_path, const ultralight::String16& destination_path)
{
  return false;
}
