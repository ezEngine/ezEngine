#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/IO/Stream.h>

#if 0 // Define to enable extensive file system profile scopes
#  define FILESYSTEM_PROFILE(szName) EZ_PROFILE_SCOPE(szName)

#else
#  define FILESYSTEM_PROFILE(Name)

#endif

/// \brief Information about a single file on disk. The file might be a document or any other file found in the data directories.
struct EZ_TOOLSFOUNDATION_DLL ezFileStatus
{
  enum class Status : ezUInt8
  {
    Unknown,    ///< Since the file has been tagged as 'Unknown' it has not been encountered again on disk (yet). Use internally to find stale entries in the model.
    FileLocked, ///< The file is locked, i.e. reading is currently not possible. Try again at a later date.
    Valid       ///< The file exists on disk.
  };

  ezFileStatus()
  {
    m_uiHash = 0;
    m_Status = Status::Unknown;
  }

  ezTimestamp m_Timestamp;
  ezUInt64 m_uiHash;
  ezUuid m_DocumentID; ///< If the file is linked to a document, the GUID is valid, otherwise not.
  Status m_Status;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_TOOLSFOUNDATION_DLL, ezFileStatus);

inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezFileStatus& value)
{
  inout_stream.WriteBytes(&value, sizeof(ezFileStatus)).IgnoreResult();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezFileStatus& ref_value)
{
  inout_stream.ReadBytes(&ref_value, sizeof(ezFileStatus));
  return inout_stream;
}
