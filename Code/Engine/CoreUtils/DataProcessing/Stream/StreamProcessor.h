
#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Reflection/Reflection.h>

class ezStreamGroup;

/// \brief Base class for all stream processor implementations.
class EZ_COREUTILS_DLL ezStreamProcessor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStreamProcessor, ezReflectedClass);

public:

  /// \brief Base constructor
  ezStreamProcessor();

  /// \brief Base destructor.
  virtual ~ezStreamProcessor();

protected:

  friend class ezStreamGroup;

  /// \brief Internal method which needs to be implemented, gets the concrete stream bindings. 
  /// This is called every time the streams are resized. Implementations should check that their required streams exist and are of the correct data types.
  virtual ezResult UpdateStreamBindings() = 0;

  /// \brief The actual method which processes the data, will be called with the number of elements to process.
  virtual void Process(ezUInt64 uiNumElements) = 0;

  /// \brief Back pointer to the stream group - will be set to the owner stream group when adding the stream processor to the group.
  /// Can be used to get stream pointers in UpdateStreamBindings();
  ezStreamGroup* m_pStreamGroup;

};
