import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

export class TestColor extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        const op1 = new ez.Color(-4.0, 0.2, -7.0, -0.0);
        const op2 = new ez.Color(2.0, 0.3, 0.0, 1.0);
        const compArray = [new ez.Color(1.0, 0.0, 0.0, 0.0), new ez.Color(0.0, 1.0, 0.0, 0.0), new ez.Color(0.0, 0.0, 1.0, 0.0), new ez.Color(0.0, 0.0, 0.0, 1.0)];

        // default constructor
        {
            let c = new ez.Color();

            EZ_TEST.FLOAT(c.r, 0);
            EZ_TEST.FLOAT(c.g, 0);
            EZ_TEST.FLOAT(c.b, 0);
            EZ_TEST.FLOAT(c.a, 1);
        }

        // constructor
        {
            let c = new ez.Color(1, 2, 3, 4);

            EZ_TEST.FLOAT(c.r, 1);
            EZ_TEST.FLOAT(c.g, 2);
            EZ_TEST.FLOAT(c.b, 3);
            EZ_TEST.FLOAT(c.a, 4);
        }

        // Clone
        {
            let c0 = new ez.Color(1, 2, 3, 4);
            let c = c0.Clone();
            c0.SetLinearRGBA(2, 3, 4, 5);

            EZ_TEST.FLOAT(c.r, 1);
            EZ_TEST.FLOAT(c.g, 2);
            EZ_TEST.FLOAT(c.b, 3);
            EZ_TEST.FLOAT(c.a, 4);
        }

        // SetColor
        {
            let c0 = new ez.Color(1, 2, 3, 4);
            let c = new ez.Color();
            c.SetColor(c0);

            EZ_TEST.FLOAT(c.r, 1);
            EZ_TEST.FLOAT(c.g, 2);
            EZ_TEST.FLOAT(c.b, 3);
            EZ_TEST.FLOAT(c.a, 4);
        }

        // ZeroColor
        {
            let c = ez.Color.ZeroColor();
            EZ_TEST.FLOAT(c.r, 0);
            EZ_TEST.FLOAT(c.g, 0);
            EZ_TEST.FLOAT(c.b, 0);
            EZ_TEST.FLOAT(c.a, 0);
        }

        // SetZero
        {
            let c = new ez.Color(1, 2, 3, 4);
            c.SetZero();

            EZ_TEST.FLOAT(c.r, 0);
            EZ_TEST.FLOAT(c.g, 0);
            EZ_TEST.FLOAT(c.b, 0);
            EZ_TEST.FLOAT(c.a, 0);
        }

        // ColorByteToFloat
        {
            EZ_TEST.FLOAT(ez.Color.ColorByteToFloat(0), 0.0, 0.000001);
            EZ_TEST.FLOAT(ez.Color.ColorByteToFloat(128), 0.501960784, 0.000001);
            EZ_TEST.FLOAT(ez.Color.ColorByteToFloat(255), 1.0, 0.000001);
        }

        // ColorFloatToByte
        {
            EZ_TEST.INT(ez.Color.ColorFloatToByte(-1.0), 0);
            EZ_TEST.INT(ez.Color.ColorFloatToByte(0.0), 0);
            EZ_TEST.INT(ez.Color.ColorFloatToByte(0.4), 102);
            EZ_TEST.INT(ez.Color.ColorFloatToByte(1.0), 255);
            EZ_TEST.INT(ez.Color.ColorFloatToByte(1.5), 255);
        }

        // SetLinearRGB / SetLinearRGBA
        {
            let c1 = new ez.Color(0, 0, 0, 0);

            c1.SetLinearRGBA(1, 2, 3, 4);

            EZ_TEST.COLOR(c1, new ez.Color(1, 2, 3, 4));

            c1.SetLinearRGB(5, 6, 7);

            EZ_TEST.COLOR(c1, new ez.Color(5, 6, 7, 4));
        }

        // IsIdenticalRGB
        {
            let c1 = new ez.Color(0, 0, 0, 0);
            let c2 = new ez.Color(0, 0, 0, 1);

            EZ_TEST.BOOL(c1.IsIdenticalRGB(c2));
            EZ_TEST.BOOL(!c1.IsIdenticalRGBA(c2));
        }

        // IsIdenticalRGBA
        {
            let tmp1 = new ez.Color();
            let tmp2 = new ez.Color();

            EZ_TEST.BOOL(op1.IsIdenticalRGBA(op1));
            for (let i = 0; i < 4; ++i) {
                tmp1.SetColor(compArray[i]);
                tmp1.MulNumber(0.001);
                tmp1.AddColor(op1);

                tmp2.SetColor(compArray[i]);
                tmp2.MulNumber(-0.001);
                tmp2.AddColor(op1);

                EZ_TEST.BOOL(!op1.IsIdenticalRGBA(tmp1));
                EZ_TEST.BOOL(!op1.IsIdenticalRGBA(tmp2));
            }
        }

        // IsEqualRGB
        {
            let c1 = new ez.Color(0, 0, 0, 0);
            let c2 = new ez.Color(0, 0, 0.2, 1);

            EZ_TEST.BOOL(!c1.IsEqualRGB(c2, 0.1));
            EZ_TEST.BOOL(c1.IsEqualRGB(c2, 0.3));
            EZ_TEST.BOOL(!c1.IsEqualRGBA(c2, 0.3));
        }

        // IsEqualRGBA
        {
            let tmp1 = new ez.Color();
            let tmp2 = new ez.Color();

            EZ_TEST.BOOL(op1.IsEqualRGBA(op1, 0.0));
            for (let i = 0; i < 4; ++i) {
                tmp1.SetColor(compArray[i]);
                tmp1.MulNumber(0.001);
                tmp1.AddColor(op1);

                tmp2.SetColor(compArray[i]);
                tmp2.MulNumber(-0.001);
                tmp2.AddColor(op1);

                EZ_TEST.BOOL(op1.IsEqualRGBA(tmp1, 2 * 0.001));
                EZ_TEST.BOOL(op1.IsEqualRGBA(tmp2, 2 * 0.001));
                EZ_TEST.BOOL(op1.IsEqualRGBA(tmp2, 2 * 0.001));
                EZ_TEST.BOOL(op1.IsEqualRGBA(tmp2, 2 * 0.001));
            }
        }

        // AddColor
        {
            let plusAssign = op1.Clone();
            plusAssign.AddColor(op2);
            EZ_TEST.BOOL(plusAssign.IsEqualRGBA(new ez.Color(-2.0, 0.5, -7.0, 1.0), 0.0001));
        }

        // SubColor
        {
            let minusAssign = op1.Clone();
            minusAssign.SubColor(op2);
            EZ_TEST.BOOL(minusAssign.IsEqualRGBA(new ez.Color(-6.0, -0.1, -7.0, -1.0), 0.0001));
        }

        // MulNumber
        {
            let mulFloat = op1.Clone();
            mulFloat.MulNumber(2.0);
            EZ_TEST.BOOL(mulFloat.IsEqualRGBA(new ez.Color(-8.0, 0.4, -14.0, -0.0), 0.0001));
            mulFloat.MulNumber(0.0);
            EZ_TEST.BOOL(mulFloat.IsEqualRGBA(new ez.Color(0.0, 0.0, 0.0, 0.0), 0.0001));
        }

        // DivNumber
        {
            let vDivFloat = op1.Clone();
            vDivFloat.DivNumber(2.0);
            EZ_TEST.BOOL(vDivFloat.IsEqualRGBA(new ez.Color(-2.0, 0.1, -3.5, -0.0), 0.0001));
        }

        // SetAdd
        {
            let plus = new ez.Color();
            plus.SetAdd(op1, op2);
            EZ_TEST.BOOL(plus.IsEqualRGBA(new ez.Color(-2.0, 0.5, -7.0, 1.0), 0.0001));
        }

        // SetSub
        {
            let minus = new ez.Color();
            minus.SetSub(op1, op2);
            EZ_TEST.BOOL(minus.IsEqualRGBA(new ez.Color(-6.0, -0.1, -7.0, -1.0), 0.0001));
        }

        // SetMulNumber
        {
            let mulVec4Float = new ez.Color();
            mulVec4Float.SetMulNumber(op1, 2);
            EZ_TEST.BOOL(mulVec4Float.IsEqualRGBA(new ez.Color(-8.0, 0.4, -14.0, -0.0), 0.0001));
            mulVec4Float.SetMulNumber(op1, 0);
            EZ_TEST.BOOL(mulVec4Float.IsEqualRGBA(new ez.Color(0.0, 0.0, 0.0, 0.0), 0.0001));
        }

        // SetDivNumber
        {
            let vDivVec4Float = new ez.Color();
            vDivVec4Float.SetDivNumber(op1, 2);
            EZ_TEST.BOOL(vDivVec4Float.IsEqualRGBA(new ez.Color(-2.0, 0.1, -3.5, -0.0), 0.0001));
        }

        // SetGammaByteRGB
        {
            let c = new ez.Color();
            c.SetGammaByteRGB(50, 100, 150);

            EZ_TEST.FLOAT(c.r, 0.031);
            EZ_TEST.FLOAT(c.g, 0.127);
            EZ_TEST.FLOAT(c.b, 0.304);
            EZ_TEST.FLOAT(c.a, 1.0);
        }

        // SetGammaByteRGBA
        {
            let c = new ez.Color();
            c.SetGammaByteRGBA(50, 100, 150, 127.5);

            EZ_TEST.FLOAT(c.r, 0.031);
            EZ_TEST.FLOAT(c.g, 0.127);
            EZ_TEST.FLOAT(c.b, 0.304);
            EZ_TEST.FLOAT(c.a, 0.5);
        }

        // ScaleRGB
        {
            let c = new ez.Color(1, 2, 3, 0.5);
            c.ScaleRGB(2);

            EZ_TEST.FLOAT(c.r, 2);
            EZ_TEST.FLOAT(c.g, 4);
            EZ_TEST.FLOAT(c.b, 6);
            EZ_TEST.FLOAT(c.a, 0.5);
        }

        // MulColor
        {
            let c = new ez.Color(2, 3, 4, 6);
            let m = new ez.Color(5, 3, 2, 0.5);

            c.MulColor(m);

            EZ_TEST.FLOAT(c.r, 10);
            EZ_TEST.FLOAT(c.g, 9);
            EZ_TEST.FLOAT(c.b, 8);
            EZ_TEST.FLOAT(c.a, 3);
        }

        // SetMul
        {
            let n = new ez.Color(2, 3, 4, 6);
            let m = new ez.Color(5, 3, 2, 0.5);

            let c = new ez.Color();
            c.SetMul(n, m);

            EZ_TEST.FLOAT(c.r, 10);
            EZ_TEST.FLOAT(c.g, 9);
            EZ_TEST.FLOAT(c.b, 8);
            EZ_TEST.FLOAT(c.a, 3);
        }

        // WithAlpha
        {
            let o = new ez.Color(2, 3, 4, 6);

            let c = o.WithAlpha(0.5);

            EZ_TEST.FLOAT(c.r, 2);
            EZ_TEST.FLOAT(c.g, 3);
            EZ_TEST.FLOAT(c.b, 4);
            EZ_TEST.FLOAT(c.a, 0.5);
        }

        // CalcAverageRGB
        {
            let c = new ez.Color(1, 1, 1, 2);
            EZ_TEST.FLOAT(c.CalcAverageRGB(), 1.0);
        }

        // IsNormalized
        {
            let c = new ez.Color(1, 1, 1, 1);
            EZ_TEST.BOOL(c.IsNormalized());

            c.a = 2.0;
            EZ_TEST.BOOL(!c.IsNormalized());
        }

        // GetLuminance
        {
            EZ_TEST.FLOAT(ez.Color.Black().GetLuminance(), 0.0);
            EZ_TEST.FLOAT(ez.Color.White().GetLuminance(), 1.0);

            EZ_TEST.FLOAT(new ez.Color(0.5, 0.5, 0.5).GetLuminance(), 0.2126 * 0.5 + 0.7152 * 0.5 + 0.0722 * 0.5);
        }

        // GetInvertedColor
        {
            let c1 = new ez.Color(0.1, 0.3, 0.7, 0.9);

            let c2 = c1.GetInvertedColor();

            EZ_TEST.BOOL(c2.IsEqualRGBA(new ez.Color(0.9, 0.7, 0.3, 0.1), 0.01));
        }

        // ComputeHdrExposureValue
        {
            let c = new ez.Color();
            EZ_TEST.FLOAT(c.ComputeHdrExposureValue(), 0);

            c.SetLinearRGB(2, 3, 4);
            EZ_TEST.FLOAT(c.ComputeHdrExposureValue(), 2);
        }

        // ApplyHdrExposureValue
        {
            let c = new ez.Color(1, 1, 1, 1);
            c.ApplyHdrExposureValue(2);

            EZ_TEST.FLOAT(c.ComputeHdrExposureValue(), 2);
        }

        // GetAsGammaByteRGBA
        {
            let c = new ez.Color();
            c.SetGammaByteRGBA(50, 100, 150, 200);

            let g = c.GetAsGammaByteRGBA();

            EZ_TEST.FLOAT(g.byteR, 50);
            EZ_TEST.FLOAT(g.byteG, 100);
            EZ_TEST.FLOAT(g.byteB, 150);
            EZ_TEST.FLOAT(g.byteA, 200);
        }
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestColor") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

