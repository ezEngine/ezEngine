#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/JSONReader.h>

/// \brief This JSON reader works mostly like its base type ezJSONReader, but also converts 'extended' types back into strongly typed ezVariant's.
///
/// This reader reads files written with ezExtendedJSONWriter and converts the extended type information and data back into a 
/// strongly typed ezVariant. Thus this format can store ints, floats, doubles, vectors, matrices, etc. without losing precision,
/// whereas standard JSON will always use doubles for all value types.
class EZ_FOUNDATION_DLL ezExtendedJSONReader : public ezJSONReader
{
protected:

  /// \brief [internal] Do not override further.
  virtual void OnEndObject() override;


};

