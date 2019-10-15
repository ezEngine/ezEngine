import __Math = require("./Math")

export import ezMath = __Math.ezMath;

export class Color {

    // TODO:
    //void SetHSV(float hue, float sat, float val)
    //void GetHSV(float& hue, float& sat, float& val) const
    //ezColor GetPaletteColor(ezUInt32 colorIndex, ezUInt8 alpha = 0xFF)
    //static ezVec3 GammaToLinear(const ezVec3& gamma)
    //static ezVec3 LinearToGamma(const ezVec3& gamma)
    //bool IsNormalized() const
    //float CalcAverageRGB() const
    //float GetSaturation() const
    //float GetLuminance() const
    //ezColor GetInvertedColor() const
    //ezColor GetComplementaryColor() const
    //float ComputeHdrMultiplier() const
    //float ComputeHdrExposureValue() const
    //void ApplyHdrExposureValue(float ev)
    //void NormalizeToLdrRange()
    //void operator*=(const ezMat4& rhs)
    //const ezColor operator*(const ezMat4& lhs, const ezColor& rhs)

    r: number;
    g: number;
    b: number;
    a: number;

    constructor(r: number = 0, g: number = 0, b: number = 0, a: number = 1) {
        this.r = r;
        this.g = g;
        this.b = b;
        this.a = a;
    }

    Clone(): Color {
        return new Color(this.r, this.g, this.b, this.a);
    }

    static ZeroColor(): Color {
        return new Color(0, 0, 0, 0);
    }

    static GammaToLinear(gamma: number): number {
        return (gamma <= 0.04045) ? (gamma / 12.92) : (Math.pow((gamma + 0.055) / 1.055, 2.4));
    }

    static LinearToGamma(linear: number): number {
        return (linear <= 0.0031308) ? (12.92 * linear) : (1.055 * Math.pow(linear, 1.0 / 2.4) - 0.055);
    }

    static ColorByteToFloat(byte: number): number {
        return byte / 255.0;
    }

    static ColorFloatToByte(float: number): number {
        return float * 255.0 + 0.5;
    }

    SetLinearRGB(r: number, g: number, b: number): void {
        this.r = r;
        this.g = g;
        this.b = b;
    }

    SetLinearRGBA(r: number, g: number, b: number, a: number): void {
        this.r = r;
        this.g = g;
        this.b = b;
        this.a = a;
    }

    SetGammaByteRGB(byteR: number, byteG: number, byteB: number): void {
        this.r = Color.GammaToLinear(Color.ColorByteToFloat(byteR));
        this.g = Color.GammaToLinear(Color.ColorByteToFloat(byteG));
        this.b = Color.GammaToLinear(Color.ColorByteToFloat(byteB));
    }

    SetGammaByteRGBA(byteR: number, byteG: number, byteB: number, byteA: number = 255): void {
        this.r = Color.GammaToLinear(Color.ColorByteToFloat(byteR));
        this.g = Color.GammaToLinear(Color.ColorByteToFloat(byteG));
        this.b = Color.GammaToLinear(Color.ColorByteToFloat(byteB));
        this.a = Color.GammaToLinear(Color.ColorByteToFloat(byteA));
    }

    SetZero(): void {
        this.r = 0;
        this.g = 0;
        this.b = 0;
        this.a = 0;
    }

    ScaleRGB(factor: number): void {
        this.r *= factor;
        this.g *= factor;
        this.b *= factor;
    }

    AddColor(rhs: Color): void {
        this.r += rhs.r;
        this.g += rhs.g;
        this.b += rhs.b;
        this.a += rhs.a;
    }

    SubColor(rhs: Color): void {
        this.r -= rhs.r;
        this.g -= rhs.g;
        this.b -= rhs.b;
        this.a -= rhs.a;
    }

    MulColor(rhs: Color): void {
        this.r *= rhs.r;
        this.g *= rhs.g;
        this.b *= rhs.b;
        this.a *= rhs.a;
    }

    MulNumber(factor: number): void {
        this.r *= factor;
        this.g *= factor;
        this.b *= factor;
        this.a *= factor;
    }

    DivNumber(factor: number): void {
        let oneDiv = 1.0 / factor;
        this.r *= oneDiv;
        this.g *= oneDiv;
        this.b *= oneDiv;
        this.a *= oneDiv;
    }

    IsIdenticalRGB(rhs: Color): boolean {
        return this.r == rhs.r && this.g == rhs.g && this.b == rhs.b;
    }

    IsIdenticalRGBA(rhs: Color): boolean {
        return this.r == rhs.r && this.g == rhs.g && this.b == rhs.b && this.a == rhs.a;
    }

    IsEqualRGB(rhs: Color, epsilon: number): boolean {
        return ezMath.IsNumberEqual(this.r, rhs.r, epsilon) && ezMath.IsNumberEqual(this.g, rhs.g, epsilon) && ezMath.IsNumberEqual(this.b, rhs.b, epsilon);
    }

    IsEqualRGBA(rhs: Color, epsilon: number): boolean {
        return ezMath.IsNumberEqual(this.r, rhs.r, epsilon) && ezMath.IsNumberEqual(this.g, rhs.g, epsilon) && ezMath.IsNumberEqual(this.b, rhs.b, epsilon) && ezMath.IsNumberEqual(this.a, rhs.a, epsilon);
    }

    WithAlpha(alpha: number): Color {
        return new Color(this.r, this.g, this.b, alpha);
    }

    SetAdd(lhs: Color, rhs: Color): void {
        this.r = lhs.r + rhs.r;
        this.g = lhs.g + rhs.g;
        this.b = lhs.b + rhs.b;
        this.a = lhs.a + rhs.a;
    }

    SetSub(lhs: Color, rhs: Color): void {
        this.r = lhs.r - rhs.r;
        this.g = lhs.g - rhs.g;
        this.b = lhs.b - rhs.b;
        this.a = lhs.a - rhs.a;
    }

    SetMul(lhs: Color, rhs: Color): void {
        this.r = lhs.r * rhs.r;
        this.g = lhs.g * rhs.g;
        this.b = lhs.b * rhs.b;
        this.a = lhs.a * rhs.a;
    }

    SetMulNumber(lhs: Color, rhs: number): void {
        this.r = lhs.r * rhs;
        this.g = lhs.g * rhs;
        this.b = lhs.b * rhs;
        this.a = lhs.a * rhs;
    }

    SetDivNumber(lhs: Color, rhs: number): void {
        let invRhs = 1.0 / rhs;
        this.r = lhs.r * invRhs;
        this.g = lhs.g * invRhs;
        this.b = lhs.b * invRhs;
        this.a = lhs.a * invRhs;
    }

    SetAliceBlue(): void { this.SetGammaByteRGBA(0xF0, 0xF8, 0xFF); }
    SetAntiqueWhite(): void { this.SetGammaByteRGBA(0xFA, 0xEB, 0xD7); }
    SetAqua(): void { this.SetGammaByteRGBA(0x00, 0xFF, 0xFF); }
    SetAquamarine(): void { this.SetGammaByteRGBA(0x7F, 0xFF, 0xD4); }
    SetAzure(): void { this.SetGammaByteRGBA(0xF0, 0xFF, 0xFF); }
    SetBeige(): void { this.SetGammaByteRGBA(0xF5, 0xF5, 0xDC); }
    SetBisque(): void { this.SetGammaByteRGBA(0xFF, 0xE4, 0xC4); }
    SetBlack(): void { this.SetGammaByteRGBA(0x00, 0x00, 0x00); }
    SetBlanchedAlmond(): void { this.SetGammaByteRGBA(0xFF, 0xEB, 0xCD); }
    SetBlue(): void { this.SetGammaByteRGBA(0x00, 0x00, 0xFF); }
    SetBlueViolet(): void { this.SetGammaByteRGBA(0x8A, 0x2B, 0xE2); }
    SetBrown(): void { this.SetGammaByteRGBA(0xA5, 0x2A, 0x2A); }
    SetBurlyWood(): void { this.SetGammaByteRGBA(0xDE, 0xB8, 0x87); }
    SetCadetBlue(): void { this.SetGammaByteRGBA(0x5F, 0x9E, 0xA0); }
    SetChartreuse(): void { this.SetGammaByteRGBA(0x7F, 0xFF, 0x00); }
    SetChocolate(): void { this.SetGammaByteRGBA(0xD2, 0x69, 0x1E); }
    SetCoral(): void { this.SetGammaByteRGBA(0xFF, 0x7F, 0x50); }
    SetCornflowerBlue(): void { this.SetGammaByteRGBA(0x64, 0x95, 0xED); } // The Original!
    SetCornsilk(): void { this.SetGammaByteRGBA(0xFF, 0xF8, 0xDC); }
    SetCrimson(): void { this.SetGammaByteRGBA(0xDC, 0x14, 0x3C); }
    SetCyan(): void { this.SetGammaByteRGBA(0x00, 0xFF, 0xFF); }
    SetDarkBlue(): void { this.SetGammaByteRGBA(0x00, 0x00, 0x8B); }
    SetDarkCyan(): void { this.SetGammaByteRGBA(0x00, 0x8B, 0x8B); }
    SetDarkGoldenRod(): void { this.SetGammaByteRGBA(0xB8, 0x86, 0x0B); }
    SetDarkGray(): void { this.SetGammaByteRGBA(0xA9, 0xA9, 0xA9); }
    SetDarkGrey(): void { this.SetGammaByteRGBA(0xA9, 0xA9, 0xA9); }
    SetDarkGreen(): void { this.SetGammaByteRGBA(0x00, 0x64, 0x00); }
    SetDarkKhaki(): void { this.SetGammaByteRGBA(0xBD, 0xB7, 0x6B); }
    SetDarkMagenta(): void { this.SetGammaByteRGBA(0x8B, 0x00, 0x8B); }
    SetDarkOliveGreen(): void { this.SetGammaByteRGBA(0x55, 0x6B, 0x2F); }
    SetDarkOrange(): void { this.SetGammaByteRGBA(0xFF, 0x8C, 0x00); }
    SetDarkOrchid(): void { this.SetGammaByteRGBA(0x99, 0x32, 0xCC); }
    SetDarkRed(): void { this.SetGammaByteRGBA(0x8B, 0x00, 0x00); }
    SetDarkSalmon(): void { this.SetGammaByteRGBA(0xE9, 0x96, 0x7A); }
    SetDarkSeaGreen(): void { this.SetGammaByteRGBA(0x8F, 0xBC, 0x8F); }
    SetDarkSlateBlue(): void { this.SetGammaByteRGBA(0x48, 0x3D, 0x8B); }
    SetDarkSlateGray(): void { this.SetGammaByteRGBA(0x2F, 0x4F, 0x4F); }
    SetDarkSlateGrey(): void { this.SetGammaByteRGBA(0x2F, 0x4F, 0x4F); }
    SetDarkTurquoise(): void { this.SetGammaByteRGBA(0x00, 0xCE, 0xD1); }
    SetDarkViolet(): void { this.SetGammaByteRGBA(0x94, 0x00, 0xD3); }
    SetDeepPink(): void { this.SetGammaByteRGBA(0xFF, 0x14, 0x93); }
    SetDeepSkyBlue(): void { this.SetGammaByteRGBA(0x00, 0xBF, 0xFF); }
    SetDimGray(): void { this.SetGammaByteRGBA(0x69, 0x69, 0x69); }
    SetDimGrey(): void { this.SetGammaByteRGBA(0x69, 0x69, 0x69); }
    SetDodgerBlue(): void { this.SetGammaByteRGBA(0x1E, 0x90, 0xFF); }
    SetFireBrick(): void { this.SetGammaByteRGBA(0xB2, 0x22, 0x22); }
    SetFloralWhite(): void { this.SetGammaByteRGBA(0xFF, 0xFA, 0xF0); }
    SetForestGreen(): void { this.SetGammaByteRGBA(0x22, 0x8B, 0x22); }
    SetFuchsia(): void { this.SetGammaByteRGBA(0xFF, 0x00, 0xFF); }
    SetGainsboro(): void { this.SetGammaByteRGBA(0xDC, 0xDC, 0xDC); }
    SetGhostWhite(): void { this.SetGammaByteRGBA(0xF8, 0xF8, 0xFF); }
    SetGold(): void { this.SetGammaByteRGBA(0xFF, 0xD7, 0x00); }
    SetGoldenRod(): void { this.SetGammaByteRGBA(0xDA, 0xA5, 0x20); }
    SetGray(): void { this.SetGammaByteRGBA(0x80, 0x80, 0x80); }
    SetGrey(): void { this.SetGammaByteRGBA(0x80, 0x80, 0x80); }
    SetGreen(): void { this.SetGammaByteRGBA(0x00, 0x80, 0x00); }
    SetGreenYellow(): void { this.SetGammaByteRGBA(0xAD, 0xFF, 0x2F); }
    SetHoneyDew(): void { this.SetGammaByteRGBA(0xF0, 0xFF, 0xF0); }
    SetHotPink(): void { this.SetGammaByteRGBA(0xFF, 0x69, 0xB4); }
    SetIndianRed(): void { this.SetGammaByteRGBA(0xCD, 0x5C, 0x5C); }
    SetIndigo(): void { this.SetGammaByteRGBA(0x4B, 0x00, 0x82); }
    SetIvory(): void { this.SetGammaByteRGBA(0xFF, 0xFF, 0xF0); }
    SetKhaki(): void { this.SetGammaByteRGBA(0xF0, 0xE6, 0x8C); }
    SetLavender(): void { this.SetGammaByteRGBA(0xE6, 0xE6, 0xFA); }
    SetLavenderBlush(): void { this.SetGammaByteRGBA(0xFF, 0xF0, 0xF5); }
    SetLawnGreen(): void { this.SetGammaByteRGBA(0x7C, 0xFC, 0x00); }
    SetLemonChiffon(): void { this.SetGammaByteRGBA(0xFF, 0xFA, 0xCD); }
    SetLightBlue(): void { this.SetGammaByteRGBA(0xAD, 0xD8, 0xE6); }
    SetLightCoral(): void { this.SetGammaByteRGBA(0xF0, 0x80, 0x80); }
    SetLightCyan(): void { this.SetGammaByteRGBA(0xE0, 0xFF, 0xFF); }
    SetLightGoldenRodYellow(): void { this.SetGammaByteRGBA(0xFA, 0xFA, 0xD2); }
    SetLightGray(): void { this.SetGammaByteRGBA(0xD3, 0xD3, 0xD3); }
    SetLightGrey(): void { this.SetGammaByteRGBA(0xD3, 0xD3, 0xD3); }
    SetLightGreen(): void { this.SetGammaByteRGBA(0x90, 0xEE, 0x90); }
    SetLightPink(): void { this.SetGammaByteRGBA(0xFF, 0xB6, 0xC1); }
    SetLightSalmon(): void { this.SetGammaByteRGBA(0xFF, 0xA0, 0x7A); }
    SetLightSeaGreen(): void { this.SetGammaByteRGBA(0x20, 0xB2, 0xAA); }
    SetLightSkyBlue(): void { this.SetGammaByteRGBA(0x87, 0xCE, 0xFA); }
    SetLightSlateGray(): void { this.SetGammaByteRGBA(0x77, 0x88, 0x99); }
    SetLightSlateGrey(): void { this.SetGammaByteRGBA(0x77, 0x88, 0x99); }
    SetLightSteelBlue(): void { this.SetGammaByteRGBA(0xB0, 0xC4, 0xDE); }
    SetLightYellow(): void { this.SetGammaByteRGBA(0xFF, 0xFF, 0xE0); }
    SetLime(): void { this.SetGammaByteRGBA(0x00, 0xFF, 0x00); }
    SetLimeGreen(): void { this.SetGammaByteRGBA(0x32, 0xCD, 0x32); }
    SetLinen(): void { this.SetGammaByteRGBA(0xFA, 0xF0, 0xE6); }
    SetMagenta(): void { this.SetGammaByteRGBA(0xFF, 0x00, 0xFF); }
    SetMaroon(): void { this.SetGammaByteRGBA(0x80, 0x00, 0x00); }
    SetMediumAquaMarine(): void { this.SetGammaByteRGBA(0x66, 0xCD, 0xAA); }
    SetMediumBlue(): void { this.SetGammaByteRGBA(0x00, 0x00, 0xCD); }
    SetMediumOrchid(): void { this.SetGammaByteRGBA(0xBA, 0x55, 0xD3); }
    SetMediumPurple(): void { this.SetGammaByteRGBA(0x93, 0x70, 0xDB); }
    SetMediumSeaGreen(): void { this.SetGammaByteRGBA(0x3C, 0xB3, 0x71); }
    SetMediumSlateBlue(): void { this.SetGammaByteRGBA(0x7B, 0x68, 0xEE); }
    SetMediumSpringGreen(): void { this.SetGammaByteRGBA(0x00, 0xFA, 0x9A); }
    SetMediumTurquoise(): void { this.SetGammaByteRGBA(0x48, 0xD1, 0xCC); }
    SetMediumVioletRed(): void { this.SetGammaByteRGBA(0xC7, 0x15, 0x85); }
    SetMidnightBlue(): void { this.SetGammaByteRGBA(0x19, 0x19, 0x70); }
    SetMintCream(): void { this.SetGammaByteRGBA(0xF5, 0xFF, 0xFA); }
    SetMistyRose(): void { this.SetGammaByteRGBA(0xFF, 0xE4, 0xE1); }
    SetMoccasin(): void { this.SetGammaByteRGBA(0xFF, 0xE4, 0xB5); }
    SetNavajoWhite(): void { this.SetGammaByteRGBA(0xFF, 0xDE, 0xAD); }
    SetNavy(): void { this.SetGammaByteRGBA(0x00, 0x00, 0x80); }
    SetOldLace(): void { this.SetGammaByteRGBA(0xFD, 0xF5, 0xE6); }
    SetOlive(): void { this.SetGammaByteRGBA(0x80, 0x80, 0x00); }
    SetOliveDrab(): void { this.SetGammaByteRGBA(0x6B, 0x8E, 0x23); }
    SetOrange(): void { this.SetGammaByteRGBA(0xFF, 0xA5, 0x00); }
    SetOrangeRed(): void { this.SetGammaByteRGBA(0xFF, 0x45, 0x00); }
    SetOrchid(): void { this.SetGammaByteRGBA(0xDA, 0x70, 0xD6); }
    SetPaleGoldenRod(): void { this.SetGammaByteRGBA(0xEE, 0xE8, 0xAA); }
    SetPaleGreen(): void { this.SetGammaByteRGBA(0x98, 0xFB, 0x98); }
    SetPaleTurquoise(): void { this.SetGammaByteRGBA(0xAF, 0xEE, 0xEE); }
    SetPaleVioletRed(): void { this.SetGammaByteRGBA(0xDB, 0x70, 0x93); }
    SetPapayaWhip(): void { this.SetGammaByteRGBA(0xFF, 0xEF, 0xD5); }
    SetPeachPuff(): void { this.SetGammaByteRGBA(0xFF, 0xDA, 0xB9); }
    SetPeru(): void { this.SetGammaByteRGBA(0xCD, 0x85, 0x3F); }
    SetPink(): void { this.SetGammaByteRGBA(0xFF, 0xC0, 0xCB); }
    SetPlum(): void { this.SetGammaByteRGBA(0xDD, 0xA0, 0xDD); }
    SetPowderBlue(): void { this.SetGammaByteRGBA(0xB0, 0xE0, 0xE6); }
    SetPurple(): void { this.SetGammaByteRGBA(0x80, 0x00, 0x80); }
    SetRebeccaPurple(): void { this.SetGammaByteRGBA(0x66, 0x33, 0x99); }
    SetRed(): void { this.SetGammaByteRGBA(0xFF, 0x00, 0x00); }
    SetRosyBrown(): void { this.SetGammaByteRGBA(0xBC, 0x8F, 0x8F); }
    SetRoyalBlue(): void { this.SetGammaByteRGBA(0x41, 0x69, 0xE1); }
    SetSaddleBrown(): void { this.SetGammaByteRGBA(0x8B, 0x45, 0x13); }
    SetSalmon(): void { this.SetGammaByteRGBA(0xFA, 0x80, 0x72); }
    SetSandyBrown(): void { this.SetGammaByteRGBA(0xF4, 0xA4, 0x60); }
    SetSeaGreen(): void { this.SetGammaByteRGBA(0x2E, 0x8B, 0x57); }
    SetSeaShell(): void { this.SetGammaByteRGBA(0xFF, 0xF5, 0xEE); }
    SetSienna(): void { this.SetGammaByteRGBA(0xA0, 0x52, 0x2D); }
    SetSilver(): void { this.SetGammaByteRGBA(0xC0, 0xC0, 0xC0); }
    SetSkyBlue(): void { this.SetGammaByteRGBA(0x87, 0xCE, 0xEB); }
    SetSlateBlue(): void { this.SetGammaByteRGBA(0x6A, 0x5A, 0xCD); }
    SetSlateGray(): void { this.SetGammaByteRGBA(0x70, 0x80, 0x90); }
    SetSlateGrey(): void { this.SetGammaByteRGBA(0x70, 0x80, 0x90); }
    SetSnow(): void { this.SetGammaByteRGBA(0xFF, 0xFA, 0xFA); }
    SetSpringGreen(): void { this.SetGammaByteRGBA(0x00, 0xFF, 0x7F); }
    SetSteelBlue(): void { this.SetGammaByteRGBA(0x46, 0x82, 0xB4); }
    SetTan(): void { this.SetGammaByteRGBA(0xD2, 0xB4, 0x8C); }
    SetTeal(): void { this.SetGammaByteRGBA(0x00, 0x80, 0x80); }
    SetThistle(): void { this.SetGammaByteRGBA(0xD8, 0xBF, 0xD8); }
    SetTomato(): void { this.SetGammaByteRGBA(0xFF, 0x63, 0x47); }
    SetTurquoise(): void { this.SetGammaByteRGBA(0x40, 0xE0, 0xD0); }
    SetViolet(): void { this.SetGammaByteRGBA(0xEE, 0x82, 0xEE); }
    SetWheat(): void { this.SetGammaByteRGBA(0xF5, 0xDE, 0xB3); }
    SetWhite(): void { this.SetGammaByteRGBA(0xFF, 0xFF, 0xFF); }
    SetWhiteSmoke(): void { this.SetGammaByteRGBA(0xF5, 0xF5, 0xF5); }
    SetYellow(): void { this.SetGammaByteRGBA(0xFF, 0xFF, 0x00); }
    SetYellowGreen(): void { this.SetGammaByteRGBA(0x9A, 0xCD, 0x32); }

}