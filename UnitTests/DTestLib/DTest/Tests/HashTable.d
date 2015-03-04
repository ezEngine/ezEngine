module DTest.Tests.HashTable;
import DTest.TestFramework;
import ez.Foundation.Basics;
import ez.Foundation.Containers.HashTable;
import DTest.ConstructionCounter;

mixin TestGroup!("Container");

struct Collision
{
  ezUInt32 hash;
  int key;

  @disable this();
  this(ezUInt32 hash, int key)
  {
    this.hash = hash;
    this.key = key;
  }

  bool opEquals(ref const(Collision) other) const
  {
    return key == other.key;
  }
}

struct CollisionHasher
{
  static ezUInt32 Hash(ref const(Collision) value)
  {
    return value.hash;
  }

  static bool Equal(ref const(Collision) a, ref const(Collision) b)
  {
    return a == b;
  }
}

@Test("HashTable")
void TestHashTable()
{
 
}