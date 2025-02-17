#include "TestFramework.as"

class ScriptObject : ezAngelScriptTestClass
{
    ScriptObject()
    {
        super("StringsTest");
    }

    private ezString m_sString = "String";
    private ezHashedString m_sHashedString = "HashedString";
    private ezTempHashedString m_sTempHashedString = "TempHashedString";
    // private ezStringView m_sStringView;          // forbidden to use as member
    // private ezStringBuilder m_sStringBuilder;    // forbidden to use as member

    bool ExecuteTests()
    {
        ezStringView svLong = "Nett hier. Aber waren sie schon mal in Baden-Württemberg?";

        // ezStringView
        {
            ezStringView sv0;
            EZ_TEST_BOOL(sv0.IsEmpty());

            ezStringView sv1("Test1");
            EZ_TEST_BOOL(!sv1.IsEmpty());
            EZ_TEST_STRING(sv1, "Test1");

            ezStringView sv2(sv1);
            EZ_TEST_BOOL(sv1 == sv2);
            EZ_TEST_BOOL(sv1 == "Test1");
            EZ_TEST_STRING(sv1, sv2);

            ezStringView sv3(m_sString);
            EZ_TEST_STRING(sv3, m_sString);

            ezStringBuilder sb1;
            sb1 = "Test1";
            EZ_TEST_STRING(sv1, sb1);
            EZ_TEST_BOOL(sv1 == sb1);

            ezStringBuilder sb2("Test2");

            ezStringView sv4(sb1);
            EZ_TEST_STRING(sv4, sb1);
            sv4 = sb2;
            EZ_TEST_STRING(sv4, sb2);
            EZ_TEST_BOOL(sv4 == sb2);
            EZ_TEST_BOOL(sv4 != sb1);

            sv1 = m_sHashedString;
            EZ_TEST_STRING(sv1, m_sHashedString.GetView());
            EZ_TEST_STRING(sv1, m_sHashedString);
            
            sv1 = m_sString;
            EZ_TEST_STRING(sv1, m_sString);

            sv1 = sb2;
            EZ_TEST_STRING(sv1, sb2);

            EZ_TEST_BOOL("Abcd" < "aBCD");
            EZ_TEST_BOOL("abc" > "ABC");
        }

        // ezString
        {
            ezString s0;
            EZ_TEST_BOOL(s0.IsEmpty());

            ezString s1("abc");
            EZ_TEST_STRING(s1, "abc");

            s1 = svLong;
            EZ_TEST_BOOL(s1 == svLong);
            EZ_TEST_STRING(s1, svLong);
            EZ_TEST_INT(s1.GetElementCount(), 58);
            
            ezString s2(s1);
            EZ_TEST_STRING(s2, svLong);

            ezStringBuilder sb1(svLong);
            EZ_TEST_INT(sb1.GetCharacterCount(), 57);

            EZ_TEST_BOOL(!s2.IsEmpty());
            s2.Clear();
            EZ_TEST_BOOL(s2.IsEmpty());

            s2 = sb1;
            EZ_TEST_STRING(s2, svLong);
            
            ezString s3(sb1);
            EZ_TEST_STRING(s3, svLong);
            s3 = m_sHashedString;
            EZ_TEST_STRING(s3, m_sHashedString);
            
            ezString s4(m_sHashedString);
            EZ_TEST_STRING(s4, m_sHashedString);

            s4 = "TEST";
            EZ_TEST_STRING(s4, "TEST");
            
            s4 = s3;
            EZ_TEST_STRING(s4, m_sHashedString);

            s3 = "ABC";
            s4 = "abc";
            EZ_TEST_BOOL(s3 < s4);
        }

        // ezStringBuilder
        {
            ezStringBuilder sb1;
            ezStringBuilder sb2("T", "e", "s", "t");
            ezStringBuilder sb3(m_sString);
            ezStringBuilder sb4(m_sHashedString);
            ezStringBuilder sb5(sb2);

            EZ_TEST_BOOL(sb1.IsEmpty());
            EZ_TEST_STRING(sb2, "Test");
            EZ_TEST_STRING(sb3, m_sString);
            EZ_TEST_STRING(sb4, m_sHashedString);
            EZ_TEST_STRING(sb5, sb2);

            sb1.SetFormat("a {} {} {} {}", 1, "b", 2.3f, ezTime::Seconds(7));
            EZ_TEST_STRING(sb1, "a 1 b 2.3 7sec");
        }
      
        // ezHashedString
        {
            ezHashedString hs1();
            EZ_TEST_BOOL(hs1.IsEmpty());
            
            ezHashedString hs2("Hallo");
            EZ_TEST_STRING(hs2, "Hallo");
            EZ_TEST_BOOL(!hs2.IsEmpty());
            
            ezHashedString hs3(hs2);
            EZ_TEST_STRING(hs2, hs3);
            EZ_TEST_BOOL(hs2 == hs3);

            hs1.Assign("Text");
            EZ_TEST_STRING(hs1, "Text");
            EZ_TEST_BOOL(hs1 != hs3);

            hs1 = "Bla";
            EZ_TEST_STRING(hs1, "Bla");
            EZ_TEST_BOOL(hs1 == "Bla");

            hs1 = m_sString;
            EZ_TEST_STRING(hs1,  m_sString);
            EZ_TEST_BOOL(!hs1.IsEmpty());
            hs1.Clear();
            EZ_TEST_BOOL(hs1.IsEmpty());

            ezTempHashedString ths = "Hallo";
            EZ_TEST_BOOL(hs2 == ths);
        }

        // ezTempHashedString
        {
            ezTempHashedString ths1;
            EZ_TEST_BOOL(ths1.IsEmpty());

            ezTempHashedString ths2(svLong);
            ezHashedString hs1(svLong);
            EZ_TEST_BOOL(hs1 == ths2);

            ezTempHashedString ths3(ths2);
            EZ_TEST_BOOL(ths2 == ths3);

            ezTempHashedString ths4(hs1);
            EZ_TEST_BOOL(ths4 == hs1);

            ths4 = "Guck guck";
            EZ_TEST_BOOL(ths4 != hs1);
            EZ_TEST_BOOL(!ths4.IsEmpty());
            
            ths4 = hs1;
            EZ_TEST_BOOL(ths4 == hs1);
            ths4.Clear();
            EZ_TEST_BOOL(ths4.IsEmpty());
        }

        return false;
    }
}