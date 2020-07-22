#pragma once

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>

/// \brief A file writer that caches all written data and only opens and writes to the output file when everything is finished.
/// Useful to ensure that only complete files are written, or nothing at all, in case of a crash.
class EZ_FOUNDATION_DLL ezDeferredFileWriter : public ezStreamWriter
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDeferredFileWriter);

public:
  ezDeferredFileWriter();

  /// \brief Upon destruction the file is closed and thus written, unless Discard was called before.
  ~ezDeferredFileWriter() { Close(); }

  /// \brief This must be configured before anything is written to the file.
  void SetOutput(const char* szFileToWriteTo, bool bOnlyWriteIfDifferent = false); // [tested]

  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) override; // [tested]

  /// \brief Upon calling this the content is written to the file specified with SetOutput().
  /// The return value is EZ_FAILURE if the file could not be opened or not completely written.
  ezResult Close(); // [tested]

  /// \brief Calling this abandons the content and a later Close or destruction of the instance
  /// will no longer write anything to file.
  void Discard(); // [tested]

private:
  bool m_bOnlyWriteIfDifferent = false;
  bool m_bAlreadyClosed = false;
  ezString m_sOutputFile;
  ezMemoryStreamStorage m_Storage;
  ezMemoryStreamWriter m_Writer;
};
