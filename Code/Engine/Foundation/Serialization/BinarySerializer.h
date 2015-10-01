#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class EZ_FOUNDATION_DLL ezAbstractGraphBinarySerializer
{
public:

  static void Write(ezStreamWriterBase& stream, const ezAbstractObjectGraph* pGraph); // [tested]
  static void Read(ezStreamReaderBase& stream, ezAbstractObjectGraph* pGraph); // [tested]

private:

};