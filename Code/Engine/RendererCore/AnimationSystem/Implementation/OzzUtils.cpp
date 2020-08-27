#include <RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
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

ezResult ezOzzArchiveData::FetchEmbeddedArchive(ezStreamReader& stream)
{
  char szTag[8] = "";

  stream.ReadBytes(szTag, 8);
  szTag[7] = '\0';

  if (!ezStringUtils::IsEqual(szTag, "ezOzzAr"))
    return EZ_FAILURE;

  /*const ezTypeVersion version =*/stream.ReadVersion(1);

  ezUInt64 uiArchiveSize = 0;
  stream >> uiArchiveSize;

  m_Storage.Clear();
  m_Storage.Reserve(uiArchiveSize);
  m_Storage.ReadAll(stream, uiArchiveSize);

  if (m_Storage.GetStorageSize() != uiArchiveSize)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezOzzArchiveData::StoreEmbeddedArchive(ezStreamWriter& stream) const
{
  const char szTag[8] = "ezOzzAr";

  EZ_SUCCEED_OR_RETURN(stream.WriteBytes(szTag, 8));

  stream.WriteVersion(1);

  const ezUInt64 uiArchiveSize = m_Storage.GetStorageSize();

  stream << uiArchiveSize;

  return stream.WriteBytes(m_Storage.GetData(), uiArchiveSize);
}

ezOzzStreamReader::ezOzzStreamReader(const ezOzzArchiveData& data)
  : m_Reader(&data.m_Storage)
{
}

bool ezOzzStreamReader::opened() const
{
  return true;
}

size_t ezOzzStreamReader::Read(void* _buffer, size_t _size)
{
  return static_cast<size_t>(m_Reader.ReadBytes(_buffer, _size));
}

size_t ezOzzStreamReader::Write(const void* _buffer, size_t _size)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

int ezOzzStreamReader::Seek(int _offset, Origin _origin)
{
  switch (_origin)
  {
    case ozz::io::Stream::kCurrent:
      m_Reader.SetReadPosition(m_Reader.GetReadPosition() + _offset);
      break;
    case ozz::io::Stream::kEnd:
      m_Reader.SetReadPosition(m_Reader.GetByteCount() - _offset);
      break;
    case ozz::io::Stream::kSet:
      m_Reader.SetReadPosition(_offset);
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

int ezOzzStreamReader::Tell() const
{
  return m_Reader.GetReadPosition();
}

size_t ezOzzStreamReader::Size() const
{
  return static_cast<size_t>(m_Reader.GetByteCount());
}

ezOzzStreamWriter::ezOzzStreamWriter(ezOzzArchiveData& data)
  : m_Writer(&data.m_Storage)
{
}

bool ezOzzStreamWriter::opened() const
{
  return true;
}

size_t ezOzzStreamWriter::Read(void* _buffer, size_t _size)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

size_t ezOzzStreamWriter::Write(const void* _buffer, size_t _size)
{
  if (m_Writer.WriteBytes(_buffer, _size).Failed())
    return 0;

  return _size;
}

int ezOzzStreamWriter::Seek(int _offset, Origin _origin)
{
  switch (_origin)
  {
    case ozz::io::Stream::kCurrent:
      m_Writer.SetWritePosition(m_Writer.GetWritePosition() + _offset);
      break;
    case ozz::io::Stream::kEnd:
      m_Writer.SetWritePosition(m_Writer.GetByteCount() - _offset);
      break;
    case ozz::io::Stream::kSet:
      m_Writer.SetWritePosition(_offset);
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

int ezOzzStreamWriter::Tell() const
{
  return m_Writer.GetWritePosition();
}

size_t ezOzzStreamWriter::Size() const
{
  return static_cast<size_t>(m_Writer.GetByteCount());
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
