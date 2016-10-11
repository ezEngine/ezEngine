
#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Reflection/Reflection.h>

class ezProcessingStreamGroup;

/// \brief Base class for all stream processor implementations.
class EZ_COREUTILS_DLL ezProcessingStreamProcessor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcessingStreamProcessor, ezReflectedClass);

public:

  /// \brief Base constructor
  ezProcessingStreamProcessor();

  /// \brief Base destructor.
  virtual ~ezProcessingStreamProcessor();

protected:

  friend class ezProcessingStreamGroup;

  /// \brief Internal method which needs to be implemented, gets the concrete stream bindings.
  /// This is called every time the streams are resized. Implementations should check that their required streams exist and are of the correct data types.
  virtual ezResult UpdateStreamBindings() = 0;

  /// \brief This method needs to be implemented in order to initialize new elements to specific values.
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) = 0;

  /// \brief The actual method which processes the data, will be called with the number of elements to process.
  virtual void Process(ezUInt64 uiNumElements) = 0;

  /// \brief Back pointer to the stream group - will be set to the owner stream group when adding the stream processor to the group.
  /// Can be used to get stream pointers in UpdateStreamBindings();
  ezProcessingStreamGroup* m_pStreamGroup;

};
