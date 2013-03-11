#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Basics/Types/Bitflags.h>

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
      Bit1 = (1 << 0),
      Bit2 = (1 << 1),
      Bit3 = (1 << 2),
      Bit4 = (1 << 3),

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

  EZ_DECLARE_FLAGS_OR_OPERATOR(ManualFlags);
}

EZ_CHECK_AT_COMPILETIME(sizeof(ezBitflags<AutoFlags>) == 4);

EZ_CREATE_SIMPLE_TEST(Basics, Bitflags)
{
  EZ_TEST(AutoFlags::Count == 4);

  ezBitflags<AutoFlags> flags = AutoFlags::Bit1 | AutoFlags::Bit4;

  EZ_TEST(flags.IsSet(AutoFlags::Bit4));
  EZ_TEST(flags.AreAllSet(AutoFlags::Bit1 | AutoFlags::Bit4));
  EZ_TEST(flags.IsAnySet(AutoFlags::Bit1 | AutoFlags::Bit2));

  flags.Add(AutoFlags::Bit3);
  EZ_TEST(flags.IsSet(AutoFlags::Bit3));

  flags.Remove(AutoFlags::Bit1);
  EZ_TEST(!flags.IsSet(AutoFlags::Bit1));

  flags.Toggle(AutoFlags::Bit4);
  EZ_TEST(flags.AreAllSet(AutoFlags::Bit3));

  flags.AddOrRemove(AutoFlags::Bit2, true);
  flags.AddOrRemove(AutoFlags::Bit3, false);
  EZ_TEST(flags.AreAllSet(AutoFlags::Bit2));

  flags.Add(AutoFlags::Bit1);

  ezBitflags<ManualFlags> manualFlags = ManualFlags::Default;
  EZ_TEST(manualFlags.AreAllSet(ManualFlags::Bit1 | ManualFlags::Bit2));
  EZ_TEST(manualFlags.GetValue() == flags.GetValue());
}
