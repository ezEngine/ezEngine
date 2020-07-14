#pragma once

namespace ezInternal
{
  constexpr ezUInt32 PRIME32_1 = 0x9E3779B1U;
  constexpr ezUInt32 PRIME32_2 = 0x85EBCA77U;
  constexpr ezUInt32 PRIME32_3 = 0xC2B2AE3DU;
  constexpr ezUInt32 PRIME32_4 = 0x27D4EB2FU;
  constexpr ezUInt32 PRIME32_5 = 0x165667B1U;

  constexpr ezUInt32 ezRotLeft(ezUInt32 value, ezUInt32 amount)
  {
    return (value << amount) | (value >> (32 - amount));
  }

  template <size_t N>
  constexpr ezUInt32 CompileTimeXxHash32(const char (&str)[N], ezUInt32 seed)
  {
    // Note: N will contain the trailing 0 of a string literal. This needs to be ignored.
    constexpr ezUInt32 length = static_cast<ezUInt32>(N - 1);
    ezUInt32 acc = 0;
    ezUInt32 index = 0;
    // Perform simple initialization if N < 16
    if constexpr (length < 16)
    {
      acc = seed + PRIME32_5;
    }
    else
    {
      ezUInt32 accs[4] = {seed + PRIME32_1 + PRIME32_2, seed + PRIME32_2, seed, seed - PRIME32_1};
      for (; length - index >= 16; index += 16)
      {
        for (int i = 0; i < 4; i++)
        {
          ezUInt32 laneN =
            (static_cast<ezUInt32>(str[index + i * 4 + 0]) << 0) |
            (static_cast<ezUInt32>(str[index + i * 4 + 1]) << 8) |
            (static_cast<ezUInt32>(str[index + i * 4 + 2]) << 16) |
            (static_cast<ezUInt32>(str[index + i * 4 + 3]) << 24);
          accs[i] = accs[i] + (laneN * PRIME32_2);
          accs[i] = ezRotLeft(accs[i], 13);
          accs[i] = accs[i] * PRIME32_1;
        }
      }
      acc = ezRotLeft(accs[0], 1) + ezRotLeft(accs[1], 7) + ezRotLeft(accs[2], 12) + ezRotLeft(accs[3], 18);
    }

    // Step 4
    acc = acc + length;

    // Step 5
    for (; length - index >= 4; index += 4)
    {
      ezUInt32 lane = (static_cast<ezUInt32>(str[index + 0]) << 0) | (static_cast<ezUInt32>(str[index + 1]) << 8) | (static_cast<ezUInt32>(str[index + 2]) << 16) | (static_cast<ezUInt32>(str[index + 3]) << 24);
      acc = acc + lane * PRIME32_3;
      acc = ezRotLeft(acc, 17) * PRIME32_4;
    }

    for (; length - index >= 1; index++)
    {
      ezUInt32 lane = static_cast<ezUInt32>(str[index]);
      acc = acc + lane * PRIME32_5;
      acc = ezRotLeft(acc, 11) * PRIME32_1;
    }

    // Step 6
    acc = acc ^ (acc >> 15);
    acc = acc * PRIME32_2;
    acc = acc ^ (acc >> 13);
    acc = acc * PRIME32_3;
    acc = acc ^ (acc >> 16);

    return acc;
  }
} // namespace ezInternal

template <size_t N>
constexpr EZ_ALWAYS_INLINE ezUInt32 ezHashingUtils::xxHash32String(const char (&str)[N], ezUInt32 uiSeed)
{
  return ezInternal::CompileTimeXxHash32(str, uiSeed);
}
