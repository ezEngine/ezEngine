#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

// Changing this value is non-trivial!
#define NUM_SWEEP_DIRECTIONS 8

// Information about a direction.
struct DirectionInfo
{
  // Direction vector (integer, in screen pixel steps)
  FLOAT2(Direction);
  // Total number of lines.
  UINT1(NumLines);
  // Offset in the line description buffer for the first line that belongs to this direction.
  UINT1(LineInstructionOffset);
};

// Information about a single line.
struct LineInstruction
{
  FLOAT2(FirstSamplePos); // Screen pixel at which this line starts.
  UINT1(LineDirIndex);    // Index that identifies the direction of this sample.
  UINT1(LineSweepOutputBufferOffset); // Index of the first output sample this line should write to.
};

CONSTANT_BUFFER(ezSSAOConstants, 3)
{
  DirectionInfo Directions[NUM_SWEEP_DIRECTIONS];
  UINT1(LineToLinePixelOffset);
  UINT1(TotalLineNumber);
};

#define SSAO_LINESWEEP_THREAD_GROUP 64