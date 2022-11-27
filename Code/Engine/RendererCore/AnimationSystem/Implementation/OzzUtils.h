#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/base/io/stream.h>

namespace ozz::animation
{
  class Skeleton;
  class Animation;
}; // namespace ozz::animation

/// \brief Stores or gather the data for an ozz file, for random access operations (seek / tell).
///
/// Since ozz::io::Stream requires seek/tell functionality, it cannot be implemented with basic ezStreamReader / ezStreamWriter.
/// Instead, we must have the entire ozz archive data in memory, to be able to jump around arbitrarily.
class EZ_RENDERERCORE_DLL ezOzzArchiveData
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezOzzArchiveData);

public:
  ezOzzArchiveData();
  ~ezOzzArchiveData();

  ezResult FetchRegularFile(const char* szFile);
  ezResult FetchEmbeddedArchive(ezStreamReader& inout_stream);
  ezResult StoreEmbeddedArchive(ezStreamWriter& inout_stream) const;

  ezDefaultMemoryStreamStorage m_Storage;
};

/// \brief Implements the ozz::io::Stream interface for reading. The data has to be present in an ezOzzArchiveData object.
///
/// The class is implemented inline and not DLL exported because ozz is only available as a static library.
class EZ_RENDERERCORE_DLL ezOzzStreamReader : public ozz::io::Stream
{
public:
  ezOzzStreamReader(const ezOzzArchiveData& data);

  virtual bool opened() const override;

  virtual size_t Read(void* pBuffer, size_t uiSize) override;

  virtual size_t Write(const void* pBuffer, size_t uiSize) override;

  virtual int Seek(int iOffset, Origin origin) override;

  virtual int Tell() const override;

  virtual size_t Size() const override;

private:
  ezMemoryStreamReader m_Reader;
};

/// \brief Implements the ozz::io::Stream interface for writing. The data is gathered in an ezOzzArchiveData object.
///
/// The class is implemented inline and not DLL exported because ozz is only available as a static library.
class EZ_RENDERERCORE_DLL ezOzzStreamWriter : public ozz::io::Stream
{
public:
  ezOzzStreamWriter(ezOzzArchiveData& ref_data);

  virtual bool opened() const override;

  virtual size_t Read(void* pBuffer, size_t uiSize) override;

  virtual size_t Write(const void* pBuffer, size_t uiSize) override;

  virtual int Seek(int iOffset, Origin origin) override;

  virtual int Tell() const override;

  virtual size_t Size() const override;

private:
  ezMemoryStreamWriter m_Writer;
};

namespace ezOzzUtils
{
  EZ_RENDERERCORE_DLL void CopyAnimation(ozz::animation::Animation* pDst, const ozz::animation::Animation* pSrc);
  EZ_RENDERERCORE_DLL void CopySkeleton(ozz::animation::Skeleton* pDst, const ozz::animation::Skeleton* pSrc);
} // namespace ezOzzUtils
