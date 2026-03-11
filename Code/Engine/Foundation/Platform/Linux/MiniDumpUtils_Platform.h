#pragma once

namespace ezMiniDumpUtils
{
  /// \brief Linux-specific implementation for writing a core dump of the running process.
  ///
  /// This triggers gcore or uses the kernel's core dump mechanism.
  EZ_FOUNDATION_DLL ezStatus WriteOwnProcessMiniDump(ezStringView sDumpFile, void* pOsSpecificData, ezDumpType dumpTypeOverride = ezDumpType::Auto);

}; // namespace ezMiniDumpUtils
