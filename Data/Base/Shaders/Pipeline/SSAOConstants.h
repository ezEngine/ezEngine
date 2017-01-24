#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

// Total number of directions in the sweep pass.
#define NUM_SWEEP_DIRECTIONS_PER_FRAME 36  // No temporal filtering, so this is also the total number of dirs!
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

  // Distance difference from which on samples are ignored.
  FLOAT1(DepthCutoffDistance);
  // How fast occlusion attenuates with distance to the occluder.
  FLOAT1(OcclusionFalloff);
};

// Notes on group size:
// Very low number proved to work nicely on GTX670
// Without shared memory in gather, 64 threads were drastically slower than 32 (2017/01/14) (almost certainly too high register pressure)
// With shared memory the difference between 128, 64 and 32 is negliable, 256 clearly worse (2017/01/15). Since AMD GCN wavefronts are 64, this is a desired minimum.
#define SSAO_LINESWEEP_THREAD_GROUP 64