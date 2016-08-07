
#pragma once

#include <CoreUtils/Basics.h>

/// \brief Base class for stream element spawners. These objects initialize newly activated elements with specific values (e.g. particle velocity, life time of a particle etc.)
class EZ_COREUTILS_DLL ezStreamElementSpawner
{
  public:

    ezStreamElementSpawner();

    virtual ~ezStreamElementSpawner();

  protected:

    friend class ezStreamGroup;

    /// \brief Internal method which needs to be implemented, gets the concrete stream bindings. 
    /// This is called every time the streams are resized. Implementations should check that their required streams exist and are of the correct data types.
    virtual ezResult UpdateStreamBindings() = 0;

    /// \brief This method needs to be implemented in order to initialize new elements to specific values.
    virtual void SpawnElements( ezUInt64 uiStartIndex, ezUInt64 uiNumElements ) = 0;

    /// \brief Back pointer to the stream group - will be set to the owner stream group when adding the stream element spawner to the group.
    /// Can be used to get stream pointers in UpdateStreamBindings();
    const ezStreamGroup* m_pStreamGroup;

};
