#pragma once

#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/AtomicInteger.h>

class ezTempHashedString;

/// \brief This class is optimized to take nearly no memory (sizeof(void*)) and to allow very fast checks whether two strings are identical.
///
/// Internally only a reference to the string data is stored. The data itself is stored in a central location, where no duplicates are
/// possible. Thus two identical strings will result in identical ezHashedString objects, which makes equality comparisons very easy
/// (it's a pointer comparison).\n
/// Copying ezHashedString objects around and assigning between them is very fast as well.\n
/// \n
/// Assigning from some other string type is rather slow though, as it requires thread synchronization.\n
/// You can also get access to the actual string data via GetString().\n
/// \n
/// You should use ezHashedString whenever the size of the encapsulating object is important and when changes to the string itself
/// are rare, but checks for equality might be frequent (e.g. in a system where objects are identified via their name).\n
/// At runtime when you need to compare ezHashedString objects with some temporary string object, used ezTempHashedString,
/// as it will only use the string's hash value for comparison, but will not store the actual string anywhere.
class EZ_FOUNDATION_DLL ezHashedString
{
public:
  struct HashedData
  {
    ezAtomicInteger32 m_iRefCount;
    ezUInt32 m_uiHash;
  };

  typedef ezMap<ezString, HashedData> StringStorage;
  typedef StringStorage::Iterator HashedType;

  /// \brief This will remove all hashed strings from the central storage, that are not referenced anymore.
  ///
  /// All hashed string values are stored in a central location and ezHashedString just references them. Those strings are then
  /// reference counted. Once some string is not referenced anymore, its ref count reaches zero, but it will not be removed from
  /// the storage, as it might be reused later again.
  /// This function will clean up all unused strings. It should typically not be necessary to call this function at all, unless lots of
  /// strings get stored in ezHashedString that are not really used throughout the applications life time.
  ///
  /// Returns the number of unused strings that were removed.
  static ezUInt32 ClearUnusedStrings();

  /// \brief Initializes this string to the empty string.
  ezHashedString(); // [tested]

  /// \brief Copies the given ezHashedString.
  ezHashedString(const ezHashedString& rhs); // [tested]

  /// \brief Releases the reference to the internal data. Does NOT deallocate any data, even if this held the last reference to some string.
  ~ezHashedString();

  /// \brief Copies the given ezHashedString.
  void operator= (const ezHashedString& rhs); // [tested]

  /// \brief Assigning a new string from a string constant is a slow operation, but the hash computation can happen at compile time.
  ///
  /// If you need to create an object to compare ezHashedString objects against, prefer to use ezTempHashedString. It will only compute
  /// the strings hash value, but does not require any thread synchronization.
  template <size_t N>
  void Assign(const char(&szString)[N]); // [tested]

  /// \brief Assigning a new string from a non-hashed string is a very slow operation, this should be used rarely.
  ///
  /// If you need to create an object to compare ezHashedString objects against, prefer to use ezTempHashedString. It will only compute
  /// the strings hash value, but does not require any thread synchronization.
  void Assign(ezHashing::StringWrapper szString); // [tested]

  /// \brief Comparing whether two hashed strings are identical is just a pointer comparison. This operation is what ezHashedString is optimized for.
  ///
  /// \note Comparing between ezHashedString objects is always error-free, so even if two string had the same hash value, although they are
  /// different, this comparison function will not report they are the same.
  bool operator== (const ezHashedString& rhs) const; // [tested]

  /// \brief \see operator==
  bool operator!= (const ezHashedString& rhs) const; // [tested]

  /// \brief Compares this string object to an ezTempHashedString object. This should be used whenever some object needs to be found
  /// and the string to compare against is not yet an ezHashedString object.
  bool operator== (const ezTempHashedString& rhs) const; // [tested]

  /// \brief \see operator==
  bool operator!= (const ezTempHashedString& rhs) const; // [tested]

  /// \brief This operator allows sorting objects by hash value, not by alphabetical order.
  bool operator< (const ezHashedString& rhs) const; // [tested]

  /// \brief This operator allows sorting objects by hash value, not by alphabetical order.
  bool operator< (const ezTempHashedString& rhs) const; // [tested]

  /// \brief Gives access to the actual string data, so you can do all the typical (read-only) string operations on it.
  const ezString& GetString() const; // [tested]

  /// \brief Gives access to the actual string data, so you can do all the typical (read-only) string operations on it.
  const char* GetData() const;

  /// \brief Returns the hash of the stored string.
  ezUInt32 GetHash() const; // [tested]

  /// \brief Returns a string view to this string's data.
  operator ezStringView() const { return GetString(); }

  /// \brief Returns a pointer to the internal Utf8 string.
  operator const char*() const { return GetData(); }

private:
  static void InitHashedString();
  static HashedType AddHashedString(const char* szString, ezUInt32 uiHash);

  HashedType m_Data;
};


/// \brief A class to use together with ezHashedString for quick comparisons with temporary strings that need not be stored further.
///
/// Whenever you have objects that use ezHashedString members and you need to compare against them with some temporary string,
/// prefer to use ezTempHashedString instead of ezHashedString, as the latter requires thread synchronization to actually set up the
/// object.
class EZ_FOUNDATION_DLL ezTempHashedString
{
  friend class ezHashedString;

  /// \brief Default constructor is disabled, you are not supposed to store these objects as members.
  ezTempHashedString();

public:
  /// \brief Creates an ezTempHashedString object from the given string constant. The hash can be computed at compile time.
  template <size_t N>
  ezTempHashedString(const char(&szString)[N]); // [tested]

  /// \brief Creates an ezTempHashedString object from the given string. Computes the hash of the given string during runtime, which might be slow.
  ezTempHashedString(ezHashing::StringWrapper szString); // [tested]

  /// \brief Copies the hash from rhs.
  ezTempHashedString(const ezTempHashedString& rhs); // [tested]

  /// \brief Copies the hash from the ezHashedString.
  ezTempHashedString(const ezHashedString& rhs); // [tested]

  /// \brief Copies the hash from the integer.
  ezTempHashedString(ezUInt32 uiHash);
  
  /// \brief The hash of the given string can be computed at compile time.
  template <size_t N>
  void operator= (const char(&szString)[N]); // [tested]
  
  /// \brief Computes and stores the hash of the given string during runtime, which might be slow.
  void operator= (ezHashing::StringWrapper szString); // [tested]

  /// \brief Copies the hash from rhs.
  void operator= (const ezTempHashedString& rhs); // [tested]

  /// \brief Copies the hash from the ezHashedString.
  void operator= (const ezHashedString& rhs); // [tested]

  /// \brief Compares the two objects by their hash value. Might report incorrect equality, if two strings have the same hash value.
  bool operator== (const ezTempHashedString& rhs) const; // [tested]

  /// \brief \see operator==
  bool operator!= (const ezTempHashedString& rhs) const; // [tested]

  /// \brief This operator allows soring objects by hash value, not by alphabetical order.
  bool operator< (const ezTempHashedString& rhs) const; // [tested]

  /// \brief Returns the hash of the stored string.
  ezUInt32 GetHash() const; // [tested]

private:
  ezUInt32 m_uiHash;
};


#include <Foundation/Strings/Implementation/HashedString_inl.h>
