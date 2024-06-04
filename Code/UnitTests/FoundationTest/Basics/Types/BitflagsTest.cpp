#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/IterateBits.h>

namespace
{
  // declare bitflags using macro magic
  EZ_DECLARE_FLAGS(ezUInt32, AutoFlags, Bit1, Bit2, Bit3, Bit4);

  // declare bitflags manually
  struct ManualFlags
  {
    using StorageType = ezUInt32;

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
} // namespace

EZ_DEFINE_AS_POD_TYPE(AutoFlags::Enum);
static_assert(sizeof(ezBitflags<AutoFlags>) == 4);


EZ_CREATE_SIMPLE_TEST(Basics, Bitflags)
{
  EZ_TEST_BOOL(AutoFlags::Count == 4);

  {
    ezBitflags<AutoFlags> flags = AutoFlags::Bit1 | AutoFlags::Bit4;

    EZ_TEST_BOOL(flags.IsSet(AutoFlags::Bit4));
    EZ_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit1 | AutoFlags::Bit4));
    EZ_TEST_BOOL(flags.IsAnySet(AutoFlags::Bit1 | AutoFlags::Bit2));
    EZ_TEST_BOOL(!flags.IsAnySet(AutoFlags::Bit2 | AutoFlags::Bit3));
    EZ_TEST_BOOL(flags.AreNoneSet(AutoFlags::Bit2 | AutoFlags::Bit3));
    EZ_TEST_BOOL(!flags.AreNoneSet(AutoFlags::Bit2 | AutoFlags::Bit4));

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
    EZ_TEST_BOOL(manualFlags.AreAllSet(ManualFlags::Default & ManualFlags::Bit2));

    EZ_TEST_BOOL(flags.IsAnyFlagSet());
    flags.Clear();
    EZ_TEST_BOOL(flags.IsNoFlagSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator&")
  {
    ezBitflags<AutoFlags> flags2 = AutoFlags::Bit1 & AutoFlags::Bit4;
    EZ_TEST_BOOL(flags2.GetValue() == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetValue")
  {
    ezBitflags<AutoFlags> flags;
    flags.SetValue(17);
    EZ_TEST_BOOL(flags.GetValue() == 17);
  }

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Iterator")
  {
    {
      // Empty
      ezBitflags<AutoFlags> f;
      auto it = f.GetIterator();
      EZ_TEST_BOOL(it == f.GetEndIterator());
      EZ_TEST_BOOL(!it.IsValid());

      for (AutoFlags::Enum flag : f)
      {
        EZ_TEST_BOOL_MSG(false, "No bit should be set");
      }
    }

    {
      // All flags
      ezBitflags<AutoFlags> f = AutoFlags::Bit1 | AutoFlags::Bit2 | AutoFlags::Bit3 | AutoFlags::Bit4;
      ezHybridArray<AutoFlags::Enum, 4> flags;
      flags.PushBack(AutoFlags::Bit1);
      flags.PushBack(AutoFlags::Bit2);
      flags.PushBack(AutoFlags::Bit3);
      flags.PushBack(AutoFlags::Bit4);

      ezUInt32 uiIndex = 0;
      // Iterator
      for (auto it = f.GetIterator(); it.IsValid(); ++it)
      {
        EZ_TEST_INT(*it, flags[uiIndex]);
        EZ_TEST_INT(it.Value(), flags[uiIndex]);
        EZ_TEST_BOOL(it.IsValid());
        ++uiIndex;
      }
      EZ_TEST_INT(uiIndex, 4);

      // Range-base for loop
      uiIndex = 0;
      for (AutoFlags::Enum flag : f)
      {
        EZ_TEST_INT(flag, flags[uiIndex]);
        ++uiIndex;
      }
      EZ_TEST_INT(uiIndex, 4);
    }
  }
}


//////////////////////////////////////////////////////////////////////////

namespace
{
  struct TypelessFlags1
  {
    enum Enum
    {
      Bit1 = EZ_BIT(0),
      Bit2 = EZ_BIT(1),
    };
  };

  struct TypelessFlags2
  {
    enum Enum
    {
      Bit3 = EZ_BIT(2),
      Bit4 = EZ_BIT(3),
    };
  };
} // namespace
