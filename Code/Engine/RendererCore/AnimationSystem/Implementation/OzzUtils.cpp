#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>

ezOzzArchiveData::ezOzzArchiveData() = default;
ezOzzArchiveData::~ezOzzArchiveData() = default;

ezResult ezOzzArchiveData::FetchRegularFile(const char* szFile)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  m_Storage.Clear();
  m_Storage.Reserve(file.GetFileSize());
  m_Storage.ReadAll(file);

  return EZ_SUCCESS;
}

ezResult ezOzzArchiveData::FetchEmbeddedArchive(ezStreamReader& inout_stream)
{
  char szTag[8] = "";

  inout_stream.ReadBytes(szTag, 8);
  szTag[7] = '\0';

  if (!ezStringUtils::IsEqual(szTag, "ezOzzAr"))
    return EZ_FAILURE;

  /*const ezTypeVersion version =*/inout_stream.ReadVersion(1);

  ezUInt64 uiArchiveSize = 0;
  inout_stream >> uiArchiveSize;

  m_Storage.Clear();
  m_Storage.Reserve(uiArchiveSize);
  m_Storage.ReadAll(inout_stream, uiArchiveSize);

  if (m_Storage.GetStorageSize64() != uiArchiveSize)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezOzzArchiveData::StoreEmbeddedArchive(ezStreamWriter& inout_stream) const
{
  const char szTag[8] = "ezOzzAr";

  EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(szTag, 8));

  inout_stream.WriteVersion(1);

  const ezUInt64 uiArchiveSize = m_Storage.GetStorageSize64();

  inout_stream << uiArchiveSize;

  return m_Storage.CopyToStream(inout_stream);
}

ezOzzStreamReader::ezOzzStreamReader(const ezOzzArchiveData& data)
  : m_Reader(&data.m_Storage)
{
}

bool ezOzzStreamReader::opened() const
{
  return true;
}

size_t ezOzzStreamReader::Read(void* pBuffer, size_t uiSize)
{
  return static_cast<size_t>(m_Reader.ReadBytes(pBuffer, uiSize));
}

size_t ezOzzStreamReader::Write(const void* pBuffer, size_t uiSize)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

int ezOzzStreamReader::Seek(int iOffset, Origin origin)
{
  switch (origin)
  {
    case ozz::io::Stream::kCurrent:
      m_Reader.SetReadPosition(m_Reader.GetReadPosition() + iOffset);
      break;
    case ozz::io::Stream::kEnd:
      m_Reader.SetReadPosition(m_Reader.GetByteCount64() - iOffset);
      break;
    case ozz::io::Stream::kSet:
      m_Reader.SetReadPosition(iOffset);
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

int ezOzzStreamReader::Tell() const
{
  return static_cast<int>(m_Reader.GetReadPosition());
}

size_t ezOzzStreamReader::Size() const
{
  return static_cast<size_t>(m_Reader.GetByteCount64());
}

ezOzzStreamWriter::ezOzzStreamWriter(ezOzzArchiveData& ref_data)
  : m_Writer(&ref_data.m_Storage)
{
}

bool ezOzzStreamWriter::opened() const
{
  return true;
}

size_t ezOzzStreamWriter::Read(void* pBuffer, size_t uiSize)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

size_t ezOzzStreamWriter::Write(const void* pBuffer, size_t uiSize)
{
  if (m_Writer.WriteBytes(pBuffer, uiSize).Failed())
    return 0;

  return uiSize;
}

int ezOzzStreamWriter::Seek(int iOffset, Origin origin)
{
  switch (origin)
  {
    case ozz::io::Stream::kCurrent:
      m_Writer.SetWritePosition(m_Writer.GetWritePosition() + iOffset);
      break;
    case ozz::io::Stream::kEnd:
      m_Writer.SetWritePosition(m_Writer.GetByteCount64() - iOffset);
      break;
    case ozz::io::Stream::kSet:
      m_Writer.SetWritePosition(iOffset);
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

int ezOzzStreamWriter::Tell() const
{
  return static_cast<int>(m_Writer.GetWritePosition());
}

size_t ezOzzStreamWriter::Size() const
{
  return static_cast<size_t>(m_Writer.GetByteCount64());
}

void ezOzzUtils::CopyAnimation(ozz::animation::Animation* pDst, const ozz::animation::Animation* pSrc)
{
  ezOzzArchiveData ozzArchiveData;

  // store in ozz archive
  {
    ezOzzStreamWriter ozzWriter(ozzArchiveData);
    ozz::io::OArchive ozzArchive(&ozzWriter);

    ozzArchive << *pSrc;
  }

  // read it from archive again
  {
    ezOzzStreamReader ozzReader(ozzArchiveData);
    ozz::io::IArchive ozzArchive(&ozzReader);

    ozzArchive >> *pDst;
  }
}

EZ_RENDERERCORE_DLL void ezOzzUtils::CopySkeleton(ozz::animation::Skeleton* pDst, const ozz::animation::Skeleton* pSrc)
{
  ezOzzArchiveData ozzArchiveData;

  // store in ozz archive
  {
    ezOzzStreamWriter ozzWriter(ozzArchiveData);
    ozz::io::OArchive ozzArchive(&ozzWriter);

    ozzArchive << *pSrc;
  }

  // read it from archive again
  {
    ezOzzStreamReader ozzReader(ozzArchiveData);
    ozz::io::IArchive ozzArchive(&ozzReader);

    ozzArchive >> *pDst;
  }
}
