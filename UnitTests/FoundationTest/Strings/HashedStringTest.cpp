#include <PCH.h>
#include <Foundation/Strings/HashedString.h>

EZ_CREATE_SIMPLE_TEST(Strings, HashedString)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezHashedString s;
    ezHashedString s2; s2.Assign("test"); // compile time hashing

    EZ_TEST_STRING(s.GetString().GetData(), "");
    EZ_TEST_BOOL(s.GetString().IsEmpty());

    ezTempHashedString ts("test"); // compile time hashing
    EZ_TEST_INT(ts.GetHash(), 0x1812752e);

    ezStringBuilder sb = "test2";
    ezTempHashedString ts2(sb.GetData()); // runtime hashing
    EZ_TEST_INT(ts2.GetHash(), 0x77e1287c);

    ezTempHashedString ts3(s2);
    EZ_TEST_INT(ts3.GetHash(), 0x1812752e);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Assign")
  {
    ezHashedString s;
    s.Assign("Test"); // compile time hashing

    EZ_TEST_STRING(s.GetString().GetData(), "Test");
    EZ_TEST_INT(s.GetHash(), 0x949e89d1);

    ezStringBuilder sb = "test2";
    s.Assign(sb.GetData()); // runtime hashing
    EZ_TEST_STRING(s.GetString().GetData(), "test2");
    EZ_TEST_INT(s.GetHash(), 0x77e1287c);
    
    ezTempHashedString ts("dummy");
    ts = "test"; // compile time hashing
    EZ_TEST_INT(ts.GetHash(), 0x1812752e);
    
    ts = sb.GetData(); // runtime hashing
    EZ_TEST_INT(ts.GetHash(), 0x77e1287c);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== / operator!=")
  {
    ezHashedString s1, s2, s3, s4;
    s1.Assign("Test1");
    s2.Assign("Test2");
    s3.Assign("Test1");
    s4.Assign("Test2");

    ezTempHashedString t1("Test1");
    ezTempHashedString t2("Test2");

    EZ_TEST_STRING(s1.GetString().GetData(), "Test1");
    EZ_TEST_STRING(s2.GetString().GetData(), "Test2");
    EZ_TEST_STRING(s3.GetString().GetData(), "Test1");
    EZ_TEST_STRING(s4.GetString().GetData(), "Test2");

    EZ_TEST_BOOL(s1 == s1);
    EZ_TEST_BOOL(s2 == s2);
    EZ_TEST_BOOL(s3 == s3);
    EZ_TEST_BOOL(s4 == s4);
    EZ_TEST_BOOL(t1 == t1);
    EZ_TEST_BOOL(t2 == t2);

    EZ_TEST_BOOL(s1 != s2);
    EZ_TEST_BOOL(s1 == s3);
    EZ_TEST_BOOL(s1 != s4);
    EZ_TEST_BOOL(s1 == t1);
    EZ_TEST_BOOL(s1 != t2);

    EZ_TEST_BOOL(s2 != s3);
    EZ_TEST_BOOL(s2 == s4);
    EZ_TEST_BOOL(s2 != t1);
    EZ_TEST_BOOL(s2 == t2);

    EZ_TEST_BOOL(s3 != s4);
    EZ_TEST_BOOL(s3 == t1);
    EZ_TEST_BOOL(s3 != t2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copying")
  {
    ezHashedString s1;
    s1.Assign("blaa");

    ezHashedString s2(s1);
    ezHashedString s3;
    s3 = s2;

    EZ_TEST_BOOL(s1 == s2);
    EZ_TEST_BOOL(s1 == s3);


    ezTempHashedString t1("blaa");

    ezTempHashedString t2(t1);
    ezTempHashedString t3("urg");
    t3 = t2;

    EZ_TEST_BOOL(t1 == t2);
    EZ_TEST_BOOL(t1 == t3);

    t3 = s1;
    EZ_TEST_INT(t3.GetHash(), s1.GetHash());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator<")
  {
    ezHashedString s1, s2, s3;
    s1.Assign("blaa");
    s2.Assign("blub");
    s3.Assign("tut");

    ezMap<ezHashedString, ezInt32> m; // uses operator< internally
    m[s1] = 1;
    m[s2] = 2;
    m[s3] = 3;

    EZ_TEST_INT(m[s1], 1);
    EZ_TEST_INT(m[s2], 2);
    EZ_TEST_INT(m[s3], 3);

    ezTempHashedString t1("blaa");
    ezTempHashedString t2("blub");
    ezTempHashedString t3("tut");

    EZ_TEST_BOOL((s1 < s1) == (t1 < t1));
    EZ_TEST_BOOL((s1 < s2) == (t1 < t2));
    EZ_TEST_BOOL((s1 < s3) == (t1 < t3));

    EZ_TEST_BOOL((s1 < s1) == (s1 < t1));
    EZ_TEST_BOOL((s1 < s2) == (s1 < t2));
    EZ_TEST_BOOL((s1 < s3) == (s1 < t3));

  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetString")
  {
    ezHashedString s1, s2, s3;
    s1.Assign("blaa");
    s2.Assign("blub");
    s3.Assign("tut");

    EZ_TEST_STRING(s1.GetString().GetData(), "blaa");
    EZ_TEST_STRING(s2.GetString().GetData(), "blub");
    EZ_TEST_STRING(s3.GetString().GetData(), "tut");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ClearUnusedStrings")
  {
    ezHashedString::ClearUnusedStrings();

    {
      ezHashedString s1, s2, s3;
      s1.Assign("blaa");
      s2.Assign("blub");
      s3.Assign("tut");
    }

    EZ_TEST_INT(ezHashedString::ClearUnusedStrings(), 3);
    EZ_TEST_INT(ezHashedString::ClearUnusedStrings(), 0);
  }

}

