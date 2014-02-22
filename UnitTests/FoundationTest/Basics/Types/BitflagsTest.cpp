#include <PCH.h>

namespace
{
  // declare bitflags using macro magic
  EZ_DECLARE_FLAGS(ezUInt32, AutoFlags, Bit1, Bit2, Bit3, Bit4);

  // declare bitflags manually
  struct ManualFlags
  {
    typedef ezUInt32 StorageType;

    enum Enum
    {
      Bit1 = EZ_BIT(0),
      Bit2 = EZ_BIT(1),
      Bit3 = EZ_BIT(2),
      Bit4 = EZ_BIT(3),

      Default = Bit1 | Bit2
    };

    struct Bits
    {
      StorageType Bit1 : 1;
      StorageType Bit2 : 1;
      StorageType Bit3 : 1;
      StorageType Bit4 : 1;
    };
  };

  EZ_DECLARE_FLAGS_OPERATORS(ManualFlags);
}

EZ_CHECK_AT_COMPILETIME(sizeof(ezBitflags<AutoFlags>) == 4);

EZ_CREATE_SIMPLE_TEST(Basics, Bitflags)
{
  EZ_TEST_BOOL(AutoFlags::Count == 4);

  ezBitflags<AutoFlags> flags = AutoFlags::Bit1 | AutoFlags::Bit4;

  EZ_TEST_BOOL(flags.IsSet(AutoFlags::Bit4));
  EZ_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit1 | AutoFlags::Bit4));
  EZ_TEST_BOOL(flags.IsAnySet(AutoFlags::Bit1 | AutoFlags::Bit2));

  flags.Add(AutoFlags::Bit3);
  EZ_TEST_BOOL(flags.IsSet(AutoFlags::Bit3));

  flags.Remove(AutoFlags::Bit1);
  EZ_TEST_BOOL(!flags.IsSet(AutoFlags::Bit1));

  flags.Toggle(AutoFlags::Bit4);
  EZ_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit3));

  flags.AddOrRemove(AutoFlags::Bit2, true);
  flags.AddOrRemove(AutoFlags::Bit3, false);
  EZ_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit2));

  flags.Add(AutoFlags::Bit1);

  ezBitflags<ManualFlags> manualFlags = ManualFlags::Default;
  EZ_TEST_BOOL(manualFlags.AreAllSet(ManualFlags::Bit1 | ManualFlags::Bit2));
  EZ_TEST_BOOL(manualFlags.GetValue() == flags.GetValue());

  ezBitflags<AutoFlags> flags2 = AutoFlags::Bit1 & AutoFlags::Bit4;
  EZ_TEST_BOOL(flags2.GetValue() == 0);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator|=")
  {
    ezBitflags<AutoFlags> f = AutoFlags::Bit1 | AutoFlags::Bit2;
    f |= AutoFlags::Bit3;

    EZ_TEST_BOOL(f.GetValue() == (AutoFlags::Bit1 | AutoFlags::Bit2 | AutoFlags::Bit3).GetValue());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator&=")
  {
    ezBitflags<AutoFlags> f = AutoFlags::Bit1 | AutoFlags::Bit2 | AutoFlags::Bit3;
    f &= AutoFlags::Bit3;

    EZ_TEST_BOOL(f.GetValue() == AutoFlags::Bit3);
  }

}
