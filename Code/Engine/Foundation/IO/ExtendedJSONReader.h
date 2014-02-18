#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/JSONReader.h>

/// \brief This JSON reader will read an entire JSON document into a hierarchical structure of ezVariants.
///
/// The reader will parse the entire document and create a data structure of ezVariants, which can then be traversed easily.
/// Note that this class is much less efficient at reading large JSON documents, as it will dynamically allocate and copy objects around
/// quite a bit. For small to medium sized documents that might be good enough, for large files one should prefer to write a dedicated
/// class derived from ezJSONParser.
class EZ_FOUNDATION_DLL ezExtendedJSONReader : public ezJSONReader
{
protected:

  /// \brief [internal] Do not override further.
  virtual void OnEndObject() EZ_OVERRIDE;


};

