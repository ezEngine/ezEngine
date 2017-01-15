#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

// Changing these values is non-trivial!
#define NUM_SWEEP_DIRECTIONS_PER_FRAME 16  // No temporal filtering, so this is also the total number of dirs!
#define NUM_SWEEP_DIRECTIONS_PER_PIXEL NUM_SWEEP_DIRECTIONS_PER_FRAME

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
  UINT1(LineDirIndex_NumSamples);    // Packed half half: Index that identifies the direction of this sample, number of samples this line has
  UINT1(LineSweepOutputBufferOffset); // Index of the first output sample this line should write to.
};

CONSTANT_BUFFER(ezSSAOConstants, 3)
{
  DirectionInfo Directions[NUM_SWEEP_DIRECTIONS_PER_FRAME];
  UINT1(LineToLinePixelOffset);
  UINT1(TotalLineNumber);
};

// Very low number prooved to work nicely on GTX670, 64 threads were drastically slower than 32 (2017/01/14)
// TODO: Isn't an AMD GCN wavefront 64 threads? This would clearly imply underutilitization. (Nvidia's warps are 32 threads)
// Note that the Gather shader has a very high register pressure and we don't need a huge number of threads!
#define SSAO_LINESWEEP_THREAD_GROUP 64