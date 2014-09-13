
#pragma once

#include <Foundation/Basics/Types/Uuid.h>

/// \brief The uuid generator abstracts safe uuid generation which uses platform specific calls.
///
/// Generating Uuids and ensuring their uniqueness is a complex task and thus the operating systems
/// available ship with library functions for this exact case. To get a uuid simply call the static NewUuid() method.
class EZ_FOUNDATION_DLL ezUuidGenerator
{
public:

  // Generates a Uuid using a syscall and thus ensuring uniqueness as good as the OS implementation [tested]
  static ezUuid NewUuid();

private:

};