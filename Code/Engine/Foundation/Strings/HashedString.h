#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
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
#if EZ_ENABLED(EZ_HASHED_STRING_REF_COUNTING)
    ezAtomicInteger32 m_iRefCount;
#endif
    ezString m_sString;
  };

  // Do NOT use a hash-table! The map does not relocate memory when it resizes, which is a vital aspect for the hashed strings to work.
  using StringStorage = ezMap<ezUInt64, HashedData, ezCompareHelper<ezUInt64>, ezStaticsAllocatorWrapper>;
  using HashedType = StringStorage::Iterator;

#if EZ_ENABLED(EZ_HASHED_STRING_REF_COUNTING)
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
#endif

  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  /// \brief Initializes this string to the empty string.
  ezHashedString(); // [tested]

  /// \brief Copies the given ezHashedString.
  ezHashedString(const ezHashedString& rhs); // [tested]

  /// \brief Moves the given ezHashedString.
  ezHashedString(ezHashedString&& rhs); // [tested]

#if EZ_ENABLED(EZ_HASHED_STRING_REF_COUNTING)
  /// \brief Releases the reference to the internal data. Does NOT deallocate any data, even if this held the last reference to some string.
  ~ezHashedString();
#endif

  /// \brief Copies the given ezHashedString.
  void operator=(const ezHashedString& rhs); // [tested]

  /// \brief Moves the given ezHashedString.
  void operator=(ezHashedString&& rhs); // [tested]

  /// \brief Assigning a new string from a string constant is a slow operation, but the hash computation can happen at compile time.
  ///
  /// If you need to create an object to compare ezHashedString objects against, prefer to use ezTempHashedString. It will only compute
  /// the strings hash value, but does not require any thread synchronization.
  template <size_t N>
  void Assign(const char (&string)[N]); // [tested]

  template <size_t N>
  void Assign(char (&string)[N]) = delete;

  /// \brief Assigning a new string from a non-hashed string is a very slow operation, this should be used rarely.
  ///
  /// If you need to create an object to compare ezHashedString objects against, prefer to use ezTempHashedString. It will only compute
  /// the strings hash value, but does not require any thread synchronization.
  void Assign(ezStringView sString); // [tested]

  /// \brief Comparing whether two hashed strings are identical is just a pointer comparison. This operation is what ezHashedString is
  /// optimized for.
  ///
  /// \note Comparing between ezHashedString objects is always error-free, so even if two string had the same hash value, although they are
  /// different, this comparison function will not report they are the same.
  bool operator==(const ezHashedString& rhs) const; // [tested]
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezHashedString&);

  /// \brief Compares this string object to an ezTempHashedString object. This should be used whenever some object needs to be found
  /// and the string to compare against is not yet an ezHashedString object.
  bool operator==(const ezTempHashedString& rhs) const; // [tested]
  bool operator!=(const ezTempHashedString& rhs) const; // [tested]

  /// \brief This operator allows sorting objects by hash value, not by alphabetical order.
  bool operator<(const ezHashedString& rhs) const; // [tested]

  /// \brief This operator allows sorting objects by hash value, not by alphabetical order.
  bool operator<(const ezTempHashedString& rhs) const; // [tested]

  /// \brief Gives access to the actual string data, so you can do all the typical (read-only) string operations on it.
  const ezString& GetString() const; // [tested]

  /// \brief Gives access to the actual string data, so you can do all the typical (read-only) string operations on it.
  const char* GetData() const;

  /// \brief Returns the hash of the stored string.
  ezUInt64 GetHash() const; // [tested]

  /// \brief Returns whether the string is empty.
  bool IsEmpty() const;

  /// \brief Resets the string to the empty string.
  void Clear();

  /// \brief Returns a string view to this string's data.
  EZ_ALWAYS_INLINE operator ezStringView() const { return GetString().GetView(); }

  /// \brief Returns a string view to this string's data.
  EZ_ALWAYS_INLINE ezStringView GetView() const { return GetString().GetView(); }

  /// \brief Returns a pointer to the internal Utf8 string.
  EZ_ALWAYS_INLINE operator const char*() const { return GetData(); }

  // since we allow to cast implicitly to const char*, we need these overloads to not do a pure pointer comparison
  EZ_ALWAYS_INLINE bool operator==(const char* szString) const { return GetString().GetView() == ezStringView(szString); }
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const char*);

  /// \brief Attempts to find a known string for the given hash value.
  ///
  /// Careful, this is a slow operation (involving a mutex). It is only meant for debug output purposes.
  /// The string hash may not be known, if the value was never assigned to any ezHashedString, in which case EZ_FAILURE is returned.
  static ezResult LookupStringHash(ezUInt64 uiHash, ezStringView& out_sResult);

private:
  static void InitHashedString();
  static HashedType AddHashedString(ezStringView sString, ezUInt64 uiHash);

  HashedType m_Data;
};

// since we allow to cast implicitly to const char*, we need these overloads to not do a pure pointer comparison
EZ_ALWAYS_INLINE bool operator==(const char* szString, const ezHashedString& rhs)
{
  return rhs.GetView() == ezStringView(szString);
}

#if EZ_DISABLED(EZ_USE_CPP20_OPERATORS)
EZ_ALWAYS_INLINE bool operator!=(const char* szString, const ezHashedString& rhs)
{
  return rhs.GetView() != ezStringView(szString);
}

#endif

/// \brief Helper function to create an ezHashedString. This can be used to initialize static hashed string variables.
template <size_t N>
ezHashedString ezMakeHashedString(const char (&string)[N]);


/// \brief A class to use together with ezHashedString for quick comparisons with temporary strings that need not be stored further.
///
/// Whenever you have objects that use ezHashedString members and you need to compare against them with some temporary string,
/// prefer to use ezTempHashedString instead of ezHashedString, as the latter requires thread synchronization to actually set up the
/// object.
class EZ_FOUNDATION_DLL ezTempHashedString
{
  friend class ezHashedString;

public:
  ezTempHashedString(); // [tested]

  /// \brief Creates an ezTempHashedString object from the given string constant. The hash can be computed at compile time.
  template <size_t N>
  ezTempHashedString(const char (&string)[N]); // [tested]

  template <size_t N>
  ezTempHashedString(char (&string)[N]) = delete;

  /// \brief Creates an ezTempHashedString object from the given string. Computes the hash of the given string during runtime, which might
  /// be slow.
  explicit ezTempHashedString(ezStringView sString); // [tested]

  /// \brief Copies the hash from rhs.
  ezTempHashedString(const ezTempHashedString& rhs); // [tested]

  /// \brief Copies the hash from the ezHashedString.
  ezTempHashedString(const ezHashedString& rhs); // [tested]

  explicit ezTempHashedString(ezUInt32 uiHash) = delete;

  /// \brief Copies the hash from the 64 bit integer.
  explicit ezTempHashedString(ezUInt64 uiHash);

  /// \brief The hash of the given string can be computed at compile time.
  template <size_t N>
  void operator=(const char (&string)[N]); // [tested]

  /// \brief Computes and stores the hash of the given string during runtime, which might be slow.
  void operator=(ezStringView sString); // [tested]

  /// \brief Copies the hash from rhs.
  void operator=(const ezTempHashedString& rhs); // [tested]

  /// \brief Copies the hash from the ezHashedString.
  void operator=(const ezHashedString& rhs); // [tested]

  /// \brief Compares the two objects by their hash value. Might report incorrect equality, if two strings have the same hash value.
  bool operator==(const ezTempHashedString& rhs) const; // [tested]
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezTempHashedString&);

  /// \brief This operator allows soring objects by hash value, not by alphabetical order.
  bool operator<(const ezTempHashedString& rhs) const; // [tested]

  /// \brief Checks whether the ezTempHashedString represents the empty string.
  bool IsEmpty() const; // [tested]

  /// \brief Resets the string to the empty string.
  void Clear(); // [tested]

  /// \brief Returns the hash of the stored string.
  ezUInt64 GetHash() const; // [tested]

  /// \brief Convenience function to call ezHashedString::LookupStringHash().
  ezResult LookupStringHash(ezStringView& out_sResult) const
  {
    return ezHashedString::LookupStringHash(m_uiHash, out_sResult);
  }

private:
  ezUInt64 m_uiHash;
};

// For ezFormatString
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezHashedString& sArg);

#include <Foundation/Strings/Implementation/HashedString_inl.h>
