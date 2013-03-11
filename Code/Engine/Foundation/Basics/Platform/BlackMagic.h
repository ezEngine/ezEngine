#pragma once

/// Gets the number of arguments of a variadic preprocessor macro
#ifndef EZ_VA_NUM_ARGS
  #define EZ_VA_NUM_ARGS(...) \
    EZ_VA_NUM_ARGS_HELPER(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

  #define EZ_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...)    N
#endif

/// Passes variadic preprocessor arguments to another macro
#ifndef EZ_PASS_VA
  #define EZ_PASS_VA(...) (__VA_ARGS__)
#endif


#define EZ_EXPAND_ARGS_1(op, a0)                                        op(a0)
#define EZ_EXPAND_ARGS_2(op, a0, a1)                                    op(a0) op(a1)
#define EZ_EXPAND_ARGS_3(op, a0, a1, a2)                                op(a0) op(a1) op(a2)
#define EZ_EXPAND_ARGS_4(op, a0, a1, a2, a3)                            op(a0) op(a1) op(a2) op(a3)
#define EZ_EXPAND_ARGS_5(op, a0, a1, a2, a3, a4)                        op(a0) op(a1) op(a2) op(a3) op(a4)
#define EZ_EXPAND_ARGS_6(op, a0, a1, a2, a3, a4, a5)                    op(a0) op(a1) op(a2) op(a3) op(a4) op(a5)
#define EZ_EXPAND_ARGS_7(op, a0, a1, a2, a3, a4, a5, a6)                op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6)
#define EZ_EXPAND_ARGS_8(op, a0, a1, a2, a3, a4, a5, a6, a7)            op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7)
#define EZ_EXPAND_ARGS_9(op, a0, a1, a2, a3, a4, a5, a6, a7, a8)        op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8)
#define EZ_EXPAND_ARGS_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9)    op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9)


#define EZ_EXPAND_ARGS_WITH_INDEX_1(op, a0)                                        op(a0, 0)
#define EZ_EXPAND_ARGS_WITH_INDEX_2(op, a0, a1)                                    op(a0, 0) op(a1, 1)
#define EZ_EXPAND_ARGS_WITH_INDEX_3(op, a0, a1, a2)                                op(a0, 0) op(a1, 1) op(a2, 2)
#define EZ_EXPAND_ARGS_WITH_INDEX_4(op, a0, a1, a2, a3)                            op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3)
#define EZ_EXPAND_ARGS_WITH_INDEX_5(op, a0, a1, a2, a3, a4)                        op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4)
#define EZ_EXPAND_ARGS_WITH_INDEX_6(op, a0, a1, a2, a3, a4, a5)                    op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5)
#define EZ_EXPAND_ARGS_WITH_INDEX_7(op, a0, a1, a2, a3, a4, a5, a6)                op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6)
#define EZ_EXPAND_ARGS_WITH_INDEX_8(op, a0, a1, a2, a3, a4, a5, a6, a7)            op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7)
#define EZ_EXPAND_ARGS_WITH_INDEX_9(op, a0, a1, a2, a3, a4, a5, a6, a7, a8)        op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 8)
#define EZ_EXPAND_ARGS_WITH_INDEX_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9)    op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 8) op(a9, 9)


/// Variadic macro "dispatching" the arguments to the correct macro.
/// The number of arguments is found by using ME_PP_NUM_ARGS(__VA_ARGS__)
#define EZ_EXPAND_ARGS(op, ...) \
  EZ_CONCAT(EZ_EXPAND_ARGS_, EZ_VA_NUM_ARGS(__VA_ARGS__)) EZ_PASS_VA(op, __VA_ARGS__)

#define EZ_EXPAND_ARGS_WITH_INDEX(op, ...) \
  EZ_CONCAT(EZ_EXPAND_ARGS_WITH_INDEX_, EZ_VA_NUM_ARGS(__VA_ARGS__)) EZ_PASS_VA(op, __VA_ARGS__)
