#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/JSONWriter.h>

/// \brief This class will redirect all calls to WriteInt32, etc. to WriteBinaryData(), meaning it will output all data using a syntax
/// similar to the MongoDB syntax for binary data.
///
/// See http://docs.mongodb.org/manual/reference/mongodb-extended-json/ for some information about the extended syntax.
///
/// Writing all values through WriteBinaryData() means that all values can be written with a type and their exact bit representation.
/// Thus JSON files written with ezExtendedJSONWriter will not lose precision and contain more information about the data types.
class EZ_FOUNDATION_DLL ezExtendedJSONWriter : public ezStandardJSONWriter
{
public:

  /// \brief Redirects to WriteBinaryData().
  virtual void WriteInt32(ezInt32 value) override; // [tested]

  /// \brief Redirects to WriteBinaryData().
  virtual void WriteUInt32(ezUInt32 value) override; // [tested]

  /// \brief Redirects to WriteBinaryData().
  virtual void WriteInt64(ezInt64 value) override; // [tested]

  /// \brief Redirects to WriteBinaryData().
  virtual void WriteUInt64(ezUInt64 value) override; // [tested]

  /// \brief Redirects to WriteBinaryData().
  virtual void WriteFloat(float value) override; // [tested]

  /// \brief Redirects to WriteBinaryData().
  virtual void WriteDouble(double value) override; // [tested]

  /// \brief Redirects to WriteBinaryData().
  virtual void WriteTime(ezTime value) override; // [tested]

};



