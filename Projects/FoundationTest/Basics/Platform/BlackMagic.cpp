#include <PCH.h>

#define INT_DECLARE(name, n) \
  int name = n;

namespace
{
  EZ_EXPAND_ARGS_WITH_INDEX(INT_DECLARE, heinz, klaus);
}

EZ_CREATE_SIMPLE_TEST(Basics, BlackMagic)
{
  EZ_TEST_INT(heinz, 0);
  EZ_TEST_INT(klaus, 1);
}
