#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/IO/Stream.h>

/// \brief A reference to a file or folder inside a data directory.
///
/// Allows quick access to various sub-parts of the path as well as the data dir index.
/// To construct a ezDataDirPath, the list of absolute data directory root directories must be supplied in order to determine whether the path is inside a data directory and in which. After calling the constructor, IsValid() should be called to determine if the file is inside a data directory.
/// The various sub-parts look like this with "Testing Chambers" being the data directory in this example:
///
///  GetAbsolutePath() == "C:/ezEngine/Data/Samples/Testing Chambers/Objects/Barrel.ezPrefab"
///  GetDataDir() == "C:/ezEngine/Data/Samples/Testing Chambers"
///  GetDataDirParentRelativePath() == "Testing Chambers/Objects/Barrel.ezPrefab"
///  GetDataDirRelativePath() == "Objects/Barrel.ezPrefab"
class EZ_TOOLSFOUNDATION_DLL ezDataDirPath
{
public:
  /// \name Constructor
  ///@{

  /// \brief Default ctor, creates an invalid data directory path.
  ezDataDirPath();
  /// \brief Tries to create a new data directory path from an absolute path. Check IsValid afterwards to confirm this path is inside a data directory.
  /// \param sAbsPath Absolute path to the file or folder. Must be normalized. Must not end with "/".
  /// \param dataDirRoots A list of normalized absolute paths to the roots of the data directories. These must not end in a "/" character.
  /// \param uiLastKnownDataDirIndex A hint to accelerate the search if the data directory index is known.
  ezDataDirPath(ezStringView sAbsPath, ezArrayPtr<ezString> dataDirRoots, ezUInt32 uiLastKnownDataDirIndex = 0);
  /// \brief Overload for ezStringBuilder to fix ambiguity between implicit conversions.
  ezDataDirPath(const ezStringBuilder& sAbsPath, ezArrayPtr<ezString> dataDirRoots, ezUInt32 uiLastKnownDataDirIndex = 0);
  /// \brief Move constructor overload for the absolute path.
  ezDataDirPath(ezString&& sAbsPath, ezArrayPtr<ezString> dataDirRoots, ezUInt32 uiLastKnownDataDirIndex = 0);


  ///@}
  /// \name Misc
  ///@{

  /// \brief Returns the same path this instance was created with. Calling this function is always valid.
  const ezString& GetAbsolutePath() const;
  /// \brief Returns whether this path is inside a data directory. If not, none of the Get* functions except for GetAbsolutePath are allowed to be called.
  bool IsValid() const;
  /// \brief Same as the default constructor. Creates an empty, invalid path.
  void Clear();

  ///@}
  /// \name Operators
  ///@{

  operator ezStringView() const;
  bool operator==(ezStringView rhs) const;
  bool operator!=(ezStringView rhs) const;

  ///@}
  /// \name Data directory access. Only allowed to be called if IsValid() is true.
  ///@{

  /// \brief Returns a relative path including the data directory the path belongs to, e.g. "Testing Chambers/Objects/Barrel.ezPrefab".
  ezStringView GetDataDirParentRelativePath() const;
  /// \brief Returns a path relative to the data directory the path belongs to, e.g. "Objects/Barrel.ezPrefab".
  ezStringView GetDataDirRelativePath() const;
  /// \brief Returns absolute path to the data directory this path belongs to, e.g.  "C:/ezEngine/Data/Samples/Testing Chambers".
  ezStringView GetDataDir() const;
  /// \brief Returns the index of the data directory the path belongs to.
  ezUInt8 GetDataDirIndex() const;

  ///@}
  /// \name Data directory update
  ///@{

  /// \brief If a ezDataDirPath is de-serialized, it might not be correct anymore and its data directory reference must be updated. It could potentially no longer be part of any data directory at all and become invalid so after calling this function, IsValid will match the return value of this function. On failure, the invalid data directory paths should then be destroyed.
  /// \param dataDirRoots A list of normalized absolute paths to the roots of the data directories. These must not end in a "/" character.
  /// \param uiLastKnownDataDirIndex A hint to accelerate the search if nothing has changed.
  /// \return Returns whether the data directory path is valid, i.e. it is still under one of the dataDirRoots.
  bool UpdateDataDirInfos(ezArrayPtr<ezString> dataDirRoots, ezUInt32 uiLastKnownDataDirIndex = 0) const;

  ///@}
  /// \name Serialization
  ///@{

  ezStreamWriter& Write(ezStreamWriter& inout_stream) const;
  ezStreamReader& Read(ezStreamReader& inout_stream);

  ///@}

private:
  ezString m_sAbsolutePath;
  mutable ezUInt16 m_uiDataDirParent = 0;
  mutable ezUInt8 m_uiDataDirLength = 0;
  mutable ezUInt8 m_uiDataDirIndex = 0;
};

ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezDataDirPath& value);
ezStreamReader& operator>>(ezStreamReader& inout_stream, ezDataDirPath& out_value);

/// \brief Comparator that first sort case-insensitive and then case-sensitive if necessary for a unique ordering.
///
/// Use this comparator when sorting e.g. files on disk like they would appear in a windows explorer.
/// This comparator is using ezStringView instead of ezDataDirPath as all string and ezDataDirPath can be implicitly converted to ezStringView.
struct EZ_TOOLSFOUNDATION_DLL ezCompareDataDirPath
{
  static inline bool Less(ezStringView lhs, ezStringView rhs);
  static inline bool Equal(ezStringView lhs, ezStringView rhs);
};

#include <ToolsFoundation/FileSystem/Implementation/DataDirPath_inl.h>
