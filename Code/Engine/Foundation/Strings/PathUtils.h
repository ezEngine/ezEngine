#pragma once

#include <Foundation/Strings/StringView.h>

class ezStringBuilder;

/// \brief Contains Helper functions to work with paths.
///
/// Only functions that require read-only access to a string are provided here
/// All functions that require to modify the path are provided by ezStringBuilder.
/// Many functions return ezStringView's, which will always be strict sub-strings of their input data.
/// That allows that these functions can work without any additional memory allocations.
class EZ_FOUNDATION_DLL ezPathUtils
{
public:
  /// \brief The path separator used by this operating system.
  static const char OsSpecificPathSeparator;

  /// \brief Returns whether c is any known path separator.
  static bool IsPathSeparator(ezUInt32 c); // [tested]

  /// \brief Checks if a given character is allowed in a filename (not path!)
  static bool IsValidFilenameChar(ezUInt32 character);

  /// \brief Checks every character in the string with IsValidFilenameChar()
  ///
  /// This is a basic check, only because each character passes the test, it does not guarantee that the full string is a valid path.
  static bool ContainsInvalidFilenameChars(const char* szPath, const char* szPathEnd = ezMaxStringEnd);

  /// \brief Searches for the previous path separator before szStartSearchAt. Will return nullptr if it reaches szPathStart before finding
  /// any separator.
  static const char* FindPreviousSeparator(const char* szPathStart, const char* szStartSearchAt); // [tested]

  /// \brief Checks whether the given path has any file extension
  static bool HasAnyExtension(const char* szPath, const char* szPathEnd = ezMaxStringEnd); // [tested]

  /// \brief Checks whether the given path ends with the given extension. szExtension should start with a '.' for performance reasons, but
  /// it will work without a '.' too.
  static bool HasExtension(const char* szPath, const char* szExtension, const char* szPathEnd = ezMaxStringEnd); // [tested]

  /// \brief Returns the file extension of the given path. Will be empty, if the path does not end with a proper extension.
  static ezStringView GetFileExtension(const char* szPath, const char* szPathEnd = ezMaxStringEnd); // [tested]

  /// \brief Returns the file name of a path, excluding the path and extension.
  ///
  /// If the path already ends with a path separator, the result will be empty.
  static ezStringView GetFileName(const char* szPath, const char* szPathEnd = ezMaxStringEnd); // [tested]

  /// \brief Returns the substring that represents the file name including the file extension.
  ///
  /// Returns an empty string, if sPath already ends in a path separator, or is empty itself.
  static ezStringView GetFileNameAndExtension(const char* szPath, const char* szPathEnd = ezMaxStringEnd); // [tested]

  /// \brief Returns the directory of the given file, which is the substring up to the last path separator.
  ///
  /// If the path already ends in a path separator, and thus points to a folder, instead of a file, the unchanged path is returned.
  /// "path/to/file" -> "path/to/"
  /// "path/to/folder/" -> "path/to/folder/"
  /// "filename" -> ""
  /// "/file_at_root_level" -> "/"
  static ezStringView GetFileDirectory(const char* szPath, const char* szPathEnd = ezMaxStringEnd); // [tested]

  /// \brief Returns true, if the given path represents an absolute path on the current OS.
  static bool IsAbsolutePath(const char* szPath); // [tested]

  /// \brief Returns true, if the given path represents a relative path on the current OS.
  static bool IsRelativePath(const char* szPath); // [tested]

  /// \brief A rooted path starts with a colon and then names a file-system data directory. Rooted paths are used as 'absolute' paths within
  /// the ezFileSystem.
  static bool IsRootedPath(const char* szPath); // [tested]

  /// \brief Splits the passed path into its root portion and the relative path
  ///
  /// ":MyRoot\file.txt" -> root = "MyRoot", relPath="file.txt"
  /// ":MyRoot\folder\file.txt" -> root = "MyRoot", relPath = "folder\file.txt"
  /// ":\MyRoot\folder\file.txt" -> root = "MyRoot", relPath = "folder\file.txt"
  /// ":/MyRoot\folder\file.txt" -> root = "MyRoot", relPath = "folder\file.txt"
  /// If the path is not rooted, then root will be an empty string and relPath is set to the full input path.
  static void GetRootedPathParts(const char* szPath, ezStringView& root, ezStringView& relPath); // [tested]

  /// \brief Special case of GetRootedPathParts that returns the root of the input path and discards the relative path
  static ezStringView GetRootedPathRootName(const char* szPath); // [tested]

  /// \brief Creates a valid filename (not path!) using the given string by replacing all disallowed characters.
  ///
  /// Note that path separators in the given string will be replaced as well!
  /// Asserts that replacementCharacter is not allowed itself.
  /// Fails for empty strings.
  /// \see IsValidFilenameChar()
  static ezResult MakeValidFilename(const char* szFilename, ezUInt32 replacementCharacter, ezStringBuilder& outFilename);
  
  /// \brief Checks whether \a sFullPath starts with \a sPrefixPath.
  static bool IsSubPath(const ezStringView& sPrefixPath, const ezStringView& sFullPath);
};

#include <Foundation/Strings/Implementation/PathUtils_inl.h>

