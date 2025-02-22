#include "TestFramework.as"

enum Phase
{
    Init,

    Done
}

class ScriptObject : ezAngelScriptTestClass
{
    private Phase m_Phase = Phase::Init;

    ScriptObject()
    {
        super("TypesTest");
    }

    bool ExecuteTests()
    {
        // These tests only test the general AS/C++ binding.
        // The are meant to find cases where the binding wasn't set up correctly, 
        // which mostly happens for operators, constructors and other special functions.
        // They do not test all the functionality, because that is already covered by the C++ tests.

        // ezAngle
        {
            ezAngle a, b;
            a = ezAngle::MakeFromDegree(90);

            EZ_TEST_FLOAT(a.GetDegree(), 90.0f);
            b = -a;

            EZ_TEST_FLOAT(b.GetDegree(), -90.0f);

            ezAngle c = a + b;
            c += a;
            c -= b;

            EZ_TEST_FLOAT(c.GetDegree(), c.GetDegree());
            EZ_TEST_BOOL(c == c);
            EZ_TEST_FLOAT(c.GetDegree(), (a * 2.0).GetDegree());
            EZ_TEST_BOOL(c == 2.0f * a);
            EZ_TEST_BOOL(c / 2.0f == a);
            EZ_TEST_BOOL(c / a == 2.0f);

            EZ_TEST_BOOL(a < c);
            EZ_TEST_BOOL(c > a);

            ezAngle d = ezAngle::AngleBetween(a, b);
            EZ_TEST_FLOAT(d.GetDegree(), 180);
        }

        // ezTime
        {
            ezTime t1 = ezTime::MakeZero();
            EZ_TEST_FLOAT(t1.AsFloatInSeconds(), 0.0f);

            t1 = ezTime::MakeFromSeconds(1.23f);
            EZ_TEST_FLOAT(t1.AsFloatInSeconds(), 1.23f);

            t1 += ezTime::Seconds(2.0f);
            t1 -= ezTime::Seconds(0.1f);
            EZ_TEST_FLOAT(t1.AsFloatInSeconds(), 3.13f);

            t1 *= 4.0f;
            t1 /= 2.0f;
            t1 = -t1;
            EZ_TEST_FLOAT(t1.AsFloatInSeconds(), -6.26f);

            ezTime t2 = ezTime::Seconds(6.26f);
            EZ_TEST_FLOAT(t1.AsFloatInSeconds(), -t2.AsFloatInSeconds());

            ezTime t3 = t1;
            EZ_TEST_BOOL(t1 == t3);
            EZ_TEST_BOOL(t1 < t2);
            EZ_TEST_BOOL(t2 > t1);

            t3 *= 2.0f;
            EZ_TEST_BOOL(t3 < t1);
            
            t3 /= 2.0f;
            EZ_TEST_FLOAT(t1.AsFloatInSeconds(), t3.AsFloatInSeconds());

            t1 = 4 * (ezTime::Seconds(2) * ezTime::Seconds(3));
            t2 = ezTime::Seconds(4) * 1.5f;
            EZ_TEST_FLOAT(t1.AsFloatInSeconds(), 24);
            EZ_TEST_FLOAT(t2.AsFloatInSeconds(), 6);
            
            t3 = t1 + t2;
            EZ_TEST_FLOAT(t3.AsFloatInSeconds(), 30);
            
            t3 = t3 - t1;
            EZ_TEST_FLOAT(t3.AsFloatInSeconds(), 6);

            t3 = t1 / t2;
            EZ_TEST_FLOAT(t3.AsFloatInSeconds(), 4);

            t3 = t1 / 3.0f;
            EZ_TEST_FLOAT(t3.AsFloatInSeconds(), 8);

            t3 = 48 / t1;
            EZ_TEST_FLOAT(t3.AsFloatInSeconds(), 2);
        }

        // ezColor / ezColorGammaUB
        {
            ezColor c1(0, 0, 0);
            EZ_TEST_COLOR(c1, ezColor::Black);

            ezColor c2(ezColorGammaUB(255, 255, 255));
            EZ_TEST_COLOR(c2, ezColor::White);

            c2 = c1;
            EZ_TEST_COLOR(c2, ezColor::Black);

            c1 = ezColorGammaUB(255, 255, 255);
            EZ_TEST_COLOR(c1, ezColor::White);

            c2 += ezColor::White;
            EZ_TEST_COLOR(c2, ezColor(1, 1, 1, 2));

            c2 -= ezColor::White;
            EZ_TEST_COLOR(c2, ezColor::Black);

            c1 *= 2;
            EZ_TEST_COLOR(c1, ezColor::White * 2);

            c1 /= 2;
            EZ_TEST_COLOR(c1, (2 * ezColor::White) / 2);

            c1 *= c1 + c1;
            EZ_TEST_COLOR(c1, ezColor::White * 2);

            c1 = c1 - c1;
            EZ_TEST_COLOR(c1, ezColor::MakeZero());

            EZ_TEST_BOOL(c1 == ezColor::MakeZero());

            EZ_TEST_VEC4(ezColor::White.WithAlpha(0.5).GetAsVec4(), ezVec4(1, 1, 1, 0.5));

            EZ_TEST_COLOR(ezColor::Red * ezColor::Blue, ezColor::Black);

            EZ_TEST_VEC4((ezColor::White / 2.0f).GetAsVec4(), ezVec4(0.5f));

            ezColorGammaUB cg = ezColor::Lime;
            EZ_TEST_INT(cg.r, 0);
            EZ_TEST_INT(cg.g, 255);
            EZ_TEST_INT(cg.b, 0);
            EZ_TEST_INT(cg.a, 255);

            cg = ezColor::Red;
            EZ_TEST_INT(cg.r, 255);
            EZ_TEST_INT(cg.g, 0);
            EZ_TEST_INT(cg.b, 0);
            EZ_TEST_INT(cg.a, 255);
        }

        // ezVec2
        {
            const ezVec2 c0 = ezVec2::MakeZero();
            const ezVec2 c1(2, 4);
            const ezVec2 c2(4, 2);
            const ezVec2 c3(6);

            EZ_TEST_FLOAT(c0.x, 0);
            EZ_TEST_FLOAT(c0.y, 0);

            EZ_TEST_FLOAT(c1.x, 2);
            EZ_TEST_FLOAT(c1.y, 4);

            EZ_TEST_FLOAT(c3.x, 6);
            EZ_TEST_FLOAT(c3.y, 6);

            EZ_TEST_BOOL(c1 != c2);
            EZ_TEST_BOOL(c1 == c1);
            EZ_TEST_BOOL(c1 < c3);
            EZ_TEST_BOOL(c3 > c1);

            ezVec2 v;
            v = c0;
            EZ_TEST_VEC2(v, c0);
            
            v += 2 * c1;
            EZ_TEST_VEC2(v, c1 * 2);
            
            v -= c1 * 2;
            EZ_TEST_VEC2(v, c0);

            v = c3;
            v *= 4;
            v /= 8;
            EZ_TEST_VEC2(v, c3 / 2);
            
            v = -1 * (c1 + c2);
            EZ_TEST_VEC2(v, -c3);
            
            v = c3 - c1;
            EZ_TEST_VEC2(v, c2);
        }

        // ezVec3
        {
            const ezVec3 c0 = ezVec3::MakeZero();
            const ezVec3 c1(2, 4, 3);
            const ezVec3 c2(4, 2, 3);
            const ezVec3 c3(6);

            EZ_TEST_FLOAT(c0.x, 0);
            EZ_TEST_FLOAT(c0.y, 0);
            EZ_TEST_FLOAT(c0.z, 0);

            EZ_TEST_FLOAT(c1.x, 2);
            EZ_TEST_FLOAT(c1.y, 4);
            EZ_TEST_FLOAT(c1.z, 3);

            EZ_TEST_FLOAT(c3.x, 6);
            EZ_TEST_FLOAT(c3.y, 6);
            EZ_TEST_FLOAT(c3.z, 6);

            EZ_TEST_BOOL(c1 != c2);
            EZ_TEST_BOOL(c1 == c1);
            EZ_TEST_BOOL(c1 < c3);
            EZ_TEST_BOOL(c3 > c1);

            ezVec3 v;
            v = c0;
            EZ_TEST_VEC3(v, c0);
            
            v += 2 * c1;
            EZ_TEST_VEC3(v, c1 * 2);
            
            v -= c1 * 2;
            EZ_TEST_VEC3(v, c0);

            v = c3;
            v *= 4;
            v /= 8;
            EZ_TEST_VEC3(v, c3 / 2);
            
            v = -1 * (c1 + c2);
            EZ_TEST_VEC3(v, -c3);
            
            v = c3 - c1;
            EZ_TEST_VEC3(v, c2);
        }

        // ezVec4
        {
            const ezVec4 c0 = ezVec4::MakeZero();
            const ezVec4 c1(2, 4, 3, 1);
            const ezVec4 c2(4, 2, 3, 5);
            const ezVec4 c3(6);

            EZ_TEST_FLOAT(c0.x, 0);
            EZ_TEST_FLOAT(c0.y, 0);
            EZ_TEST_FLOAT(c0.z, 0);
            EZ_TEST_FLOAT(c0.w, 0);

            EZ_TEST_FLOAT(c1.x, 2);
            EZ_TEST_FLOAT(c1.y, 4);
            EZ_TEST_FLOAT(c1.z, 3);
            EZ_TEST_FLOAT(c1.w, 1);

            EZ_TEST_FLOAT(c3.x, 6);
            EZ_TEST_FLOAT(c3.y, 6);
            EZ_TEST_FLOAT(c3.y, 6);
            EZ_TEST_FLOAT(c3.y, 6);

            EZ_TEST_BOOL(c1 != c2);
            EZ_TEST_BOOL(c1 == c1);
            EZ_TEST_BOOL(c1 < c3);
            EZ_TEST_BOOL(c3 > c1);

            ezVec4 v;
            v = c0;
            EZ_TEST_VEC4(v, c0);
            
            v += 2 * c1;
            EZ_TEST_VEC4(v, c1 * 2);
            
            v -= c1 * 2;
            EZ_TEST_VEC4(v, c0);

            v = c3;
            v *= 4;
            v /= 8;
            EZ_TEST_VEC4(v, c3 / 2);
            
            v = -1 * (c1 + c2);
            EZ_TEST_VEC4(v, -c3);
            
            v = c3 - c1;
            EZ_TEST_VEC4(v, c2);
        }

        // ezQuat
        {
            EZ_TEST_QUAT(ezQuat::MakeIdentity(), ezQuat::MakeFromElements(0, 0, 0, 1));

            ezQuat q = ezQuat::MakeFromElements(1, 2, 3, 4);
            EZ_TEST_FLOAT(q.x, 1);
            EZ_TEST_FLOAT(q.y, 2);
            EZ_TEST_FLOAT(q.z, 3);
            EZ_TEST_FLOAT(q.w, 4);

            EZ_TEST_BOOL(q == q);
            EZ_TEST_BOOL(q != q.GetNegated());

            ezQuat q2 = q;
            EZ_TEST_BOOL(q2 == q);

            q2 = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(90));
            ezVec3 v = q2 * ezVec3(1, 2, 3);

            EZ_TEST_VEC3(v, ezVec3(1, -3, 2));

            q = q2 * q2;

            ezVec3 axis;
            ezAngle angle;
            q.GetRotationAxisAndAngle(axis, angle);

            EZ_TEST_VEC3(axis, ezVec3(1, 0, 0));
            EZ_TEST_FLOAT(angle.GetDegree(), 180);
        }

        // ezTransform
        {
            ezTransform t1 = ezTransform::MakeIdentity();
            EZ_TEST_VEC3(t1.m_vPosition, ezVec3::MakeZero());
            EZ_TEST_QUAT(t1.m_qRotation, ezQuat::MakeIdentity());
            EZ_TEST_VEC3(t1.m_vScale, ezVec3(1));

            EZ_TEST_BOOL(t1 == t1.GetInverse());
            
            ezTransform t2;
            t2 = t1;

            EZ_TEST_BOOL(t1 == t2);
            
            t1 += ezVec3(1);
            EZ_TEST_BOOL(t1 != t2);
            EZ_TEST_BOOL(t1 == t2 + ezVec3(1));
            EZ_TEST_BOOL(t1 - ezVec3(1) == t2);

            t1 -= ezVec3(1);
            EZ_TEST_BOOL(t1 == t2);

            EZ_TEST_BOOL(ezTransform::Make(ezVec3(1), ezQuat::MakeIdentity(), ezVec3(2)) == ezTransform(ezVec3(1), ezQuat::MakeIdentity(), ezVec3(2)));

            t1 = ezTransform::Make(ezVec3(1, 2, 3));
            ezVec3 pos(2, 3, 4);
            pos = t1 * pos;
            EZ_TEST_VEC3(pos, ezVec3(3, 5, 7));

            ezQuat rot = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(90));
            t1 = t1 * rot;
            t1 = rot * t1;

            ezTransform t3 = t1 * t2;
            ezTransform t4 = ezTransform::MakeGlobalTransform(t1, t2);
            EZ_TEST_BOOL(t3 == t4);
        }

        // ezMath
        {
            // nothing that needs testing
        }

        return false;
    }
}