import __Utils = require("./Utils")

export import Utils = __Utils.Utils;

/**
 * Represents an HDR RGBA color value in linear space.
 */
export class Color {

    // TODO:
    //ezColor GetPaletteColor(ezUInt32 colorIndex, ezUInt8 alpha = 0xFF)
    //void SetHSV(float hue, float sat, float val)
    //void GetHSV(float& hue, float& sat, float& val) const
    //float GetSaturation() const
    //ezColor GetComplementaryColor() const

    /** The red value */
    r: number;
    /** The green value */
    g: number;
    /** The blue value */
    b: number;
    /** The alpha value */
    a: number;

    /**
     * Constructs a custom color or default initializes it to black (alpha = 1)
     * 
     * @param r Red in [0; 1] linear range.
     * @param g Green in [0; 1] linear range.
     * @param b Blue in [0; 1] linear range.
     * @param a Alpha in [0; 1] range.
     */
    constructor(r: number = 0, g: number = 0, b: number = 0, a: number = 1) { // [tested]
        this.r = r;
        this.g = g;
        this.b = b;
        this.a = a;
    }

    /**
     * Returns a duplicate of this object.
     */
    Clone(): Color { // [tested]
        return new Color(this.r, this.g, this.b, this.a);
    }

    SetColor(rhs: Color) { // [tested]
        this.r = rhs.r;
        this.g = rhs.g;
        this.b = rhs.b;
        this.a = rhs.a;
    }

    /**
     * Returns an all-zero color object.
     */
    static ZeroColor(): Color { // [tested]
        return new Color(0, 0, 0, 0);
    }

    /**
     * Converts a color value from Gamma space to Linear space.
     * 
     * @param gamma A color value (red, green or blue) in Gamma space.
     * @returns The converted value in Linear space.
     */
    static GammaToLinear(gamma: number): number { // [tested]
        return (gamma <= 0.04045) ? (gamma / 12.92) : (Math.pow((gamma + 0.055) / 1.055, 2.4));
    }

    /**
     * Converts a color value from Linear space to Gamma space.
     * 
     * @param linear A color value (red, green or blue) in Linear space.
     * @returns The converted value in Gamma space.
     */
    static LinearToGamma(linear: number): number { // [tested]
        return (linear <= 0.0031308) ? (12.92 * linear) : (1.055 * Math.pow(linear, 1.0 / 2.4) - 0.055);
    }

    /**
     * Converts a color value from [0; 255] range to [0; 1] range.
     * 
     * @param byte A color value in [0; 255] range.
     * @returns The color value in [0; 1] range.
     */
    static ColorByteToFloat(byte: number): number { // [tested]
        return byte / 255.0;
    }

    /**
     * Converts a color value from [0; 1] range to [0; 255] range.
     * 
     * @param float A color value in [0; 1] range.
     * @returns The color value in [0; 255] range.
     */
    static ColorFloatToByte(float: number): number { // [tested]
        return Math.floor(Utils.Saturate(float) * 255.0 + 0.5);
    }

    /**
     * Sets the RGB part with values in Linear space, [0; 1] range.
     * Does not modify alpha.
     */
    SetLinearRGB(r: number, g: number, b: number): void { // [tested]
        this.r = r;
        this.g = g;
        this.b = b;
    }

    /**
     * Sets RGB and Alpha values in Linear space, [0; 1] range.
     */
    SetLinearRGBA(r: number, g: number, b: number, a: number = 1.0): void { // [tested]
        this.r = r;
        this.g = g;
        this.b = b;
        this.a = a;
    }

    /**
     * Sets the RGB part with values in Gamma space, [0; 255] range.
     * Converts the given values to Linear space, [0; 1] range.
     * Does not modify alpha.
     */
    SetGammaByteRGB(byteR: number, byteG: number, byteB: number): void { // [tested]
        this.r = Color.GammaToLinear(Color.ColorByteToFloat(byteR));
        this.g = Color.GammaToLinear(Color.ColorByteToFloat(byteG));
        this.b = Color.GammaToLinear(Color.ColorByteToFloat(byteB));
    }

    /**
     * Sets RGB and Alpha with values in Gamma space, [0; 255] range.
     * Converts the given values to Linear space, [0; 1] range.
     */
    SetGammaByteRGBA(byteR: number, byteG: number, byteB: number, byteA: number = 255): void { // [tested]
        this.r = Color.GammaToLinear(Color.ColorByteToFloat(byteR));
        this.g = Color.GammaToLinear(Color.ColorByteToFloat(byteG));
        this.b = Color.GammaToLinear(Color.ColorByteToFloat(byteB));
        this.a = Color.ColorByteToFloat(byteA);
    }

    /**
     * Returns RGBA converted to GAmma space and in [0; 255] range.
     */
    GetAsGammaByteRGBA(): { byteR: number, byteG: number, byteB: number, byteA: number } { // [tested]

        let r = Color.ColorFloatToByte(Color.LinearToGamma(this.r));
        let g = Color.ColorFloatToByte(Color.LinearToGamma(this.g));
        let b = Color.ColorFloatToByte(Color.LinearToGamma(this.b));
        let a = Color.ColorFloatToByte(this.a);

        return { byteR: r, byteG: g, byteB: b, byteA: a };
    }

    /**
     * Sets all values to zero.
     */
    SetZero(): void { // [tested]
        this.r = 0;
        this.g = 0;
        this.b = 0;
        this.a = 0;
    }

    /**
     * Scales the RGB values with the given factor.
     * Does not modify Alpha.
     */
    ScaleRGB(factor: number): void { // [tested]
        this.r *= factor;
        this.g *= factor;
        this.b *= factor;
    }

    /**
     * Adds rhs to this.
     */
    AddColor(rhs: Color): void { // [tested]
        this.r += rhs.r;
        this.g += rhs.g;
        this.b += rhs.b;
        this.a += rhs.a;
    }

    /**
     * Subtracts rhs from this.
     */
    SubColor(rhs: Color): void { // [tested]
        this.r -= rhs.r;
        this.g -= rhs.g;
        this.b -= rhs.b;
        this.a -= rhs.a;
    }

    /**
     * Multiplies rhs into this.
     */
    MulColor(rhs: Color): void { // [tested]
        this.r *= rhs.r;
        this.g *= rhs.g;
        this.b *= rhs.b;
        this.a *= rhs.a;
    }

    /**
     * Multiplies this with factor.
     */
    MulNumber(factor: number): void { // [tested]
        this.r *= factor;
        this.g *= factor;
        this.b *= factor;
        this.a *= factor;
    }

    /**
     * Divides this by factor.
     */
    DivNumber(factor: number): void { // [tested]
        let oneDiv = 1.0 / factor;
        this.r *= oneDiv;
        this.g *= oneDiv;
        this.b *= oneDiv;
        this.a *= oneDiv;
    }

    /**
     * Checks whether this and rhs are completely identical in RGB. Ignores Alpha.
     */
    IsIdenticalRGB(rhs: Color): boolean { // [tested]
        return this.r == rhs.r && this.g == rhs.g && this.b == rhs.b;
    }

    /**
     * Checks whether this and rhs are completely identical in RGB and Alpha.
     */
    IsIdenticalRGBA(rhs: Color): boolean { // [tested]
        return this.r == rhs.r && this.g == rhs.g && this.b == rhs.b && this.a == rhs.a;
    }

    /**
     * Checks whether this and rhs are approximately equal in RGB. Ignores Alpha.
     * @param epsilon In Linear space, [0; 1] range.
     */
    IsEqualRGB(rhs: Color, epsilon: number = 0.01): boolean { // [tested]
        return Utils.IsNumberEqual(this.r, rhs.r, epsilon) && Utils.IsNumberEqual(this.g, rhs.g, epsilon) && Utils.IsNumberEqual(this.b, rhs.b, epsilon);
    }

    /**
     * Checks whether this and rhs are approximately equal in RGB and Alpha.
     * @param epsilon In Linear space, [0; 1] range.
     */
    IsEqualRGBA(rhs: Color, epsilon: number = 0.01): boolean { // [tested]
        return Utils.IsNumberEqual(this.r, rhs.r, epsilon) && Utils.IsNumberEqual(this.g, rhs.g, epsilon) && Utils.IsNumberEqual(this.b, rhs.b, epsilon) && Utils.IsNumberEqual(this.a, rhs.a, epsilon);
    }

    /**
     * Returns a duplicate of this, but with a replaced alpha value.
     */
    WithAlpha(alpha: number): Color { // [tested]
        return new Color(this.r, this.g, this.b, alpha);
    }

    /**
     * Sets this to lhs + rhs.
     */
    SetAdd(lhs: Color, rhs: Color): void { // [tested]
        this.r = lhs.r + rhs.r;
        this.g = lhs.g + rhs.g;
        this.b = lhs.b + rhs.b;
        this.a = lhs.a + rhs.a;
    }

    /**
     * Sets this to lhs - rhs.
     */
    SetSub(lhs: Color, rhs: Color): void { // [tested]
        this.r = lhs.r - rhs.r;
        this.g = lhs.g - rhs.g;
        this.b = lhs.b - rhs.b;
        this.a = lhs.a - rhs.a;
    }

    /**
     * Sets this to lhs * rhs.
     */
    SetMul(lhs: Color, rhs: Color): void { // [tested]
        this.r = lhs.r * rhs.r;
        this.g = lhs.g * rhs.g;
        this.b = lhs.b * rhs.b;
        this.a = lhs.a * rhs.a;
    }

    /**
     * Sets this to lhs * rhs.
     */
    SetMulNumber(lhs: Color, rhs: number): void { // [tested]
        this.r = lhs.r * rhs;
        this.g = lhs.g * rhs;
        this.b = lhs.b * rhs;
        this.a = lhs.a * rhs;
    }

    /**
     * Sets this to lhs / rhs.
     */
    SetDivNumber(lhs: Color, rhs: number): void { // [tested]
        let invRhs = 1.0 / rhs;
        this.r = lhs.r * invRhs;
        this.g = lhs.g * invRhs;
        this.b = lhs.b * invRhs;
        this.a = lhs.a * invRhs;
    }

    /**
     * Calculates the average of the RGB channels.
     */
    CalcAverageRGB(): number // [tested]
    {
        return (this.r + this.g + this.b) / 3.0;
    }

    /**
     * Returns if the color is in the Range [0; 1] on all 4 channels.
     */
    IsNormalized(): boolean { // [tested]

        return (this.r <= 1.0 && this.g <= 1.0 && this.b <= 1.0 && this.a <= 1.0) &&
            (this.r >= 0.0 && this.g >= 0.0 && this.b >= 0.0 && this.a >= 0.0);
    }

    /**
     * Returns 1 for an LDR color (all ´RGB components < 1). Otherwise the value of the largest component. Ignores alpha.
     */
    ComputeHdrMultiplier(): number { // [tested]
        return Math.max(1.0, Math.max(this.r, Math.max(this.g, this.b)));
    }

    /**
     * If this is an HDR color, the largest component value is used to normalize RGB to LDR range. Alpha is unaffected.
     */
    NormalizeToLdrRange(): void {
        this.ScaleRGB(1.0 / this.ComputeHdrMultiplier());
    }

    /**
     * Computes the perceived luminance. Assumes linear color space (http://en.wikipedia.org/wiki/Luminance_%28relative%29).
     */
    GetLuminance(): number // [tested]
    {
        return 0.2126 * this.r + 0.7152 * this.g + 0.0722 * this.b;
    }

    /**
     * Performs a simple (1.0 - color) inversion on all four channels.
     */
    GetInvertedColor(): Color { // [tested]

        return new Color(1.0 - this.r, 1.0 - this.g, 1.0 - this.b, 1.0 - this.a);
    }

    /**
     * Returns the base-2 logarithm of ComputeHdrMultiplier().
     * 0 for LDR colors, +1, +2, etc. for HDR colors.
     */
    ComputeHdrExposureValue(): number // [tested]
    {
        return Math.log(this.ComputeHdrMultiplier()) / Math.log(2.0);
    }

    /**
     * Raises 2 to the power \a ev and multiplies RGB with that factor.
     */
    ApplyHdrExposureValue(ev: number): void // [tested]
    {
        const factor = Math.pow(2, ev);
        this.r *= factor;
        this.g *= factor;
        this.b *= factor;
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

    static AliceBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0xF0, 0xF8, 0xFF); return c; }
    static AntiqueWhite(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFA, 0xEB, 0xD7); return c; }
    static Aqua(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0xFF, 0xFF); return c; }
    static Aquamarine(): Color { let c = new Color(); c.SetGammaByteRGBA(0x7F, 0xFF, 0xD4); return c; }
    static Azure(): Color { let c = new Color(); c.SetGammaByteRGBA(0xF0, 0xFF, 0xFF); return c; }
    static Beige(): Color { let c = new Color(); c.SetGammaByteRGBA(0xF5, 0xF5, 0xDC); return c; }
    static Bisque(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xE4, 0xC4); return c; }
    static Black(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0x00, 0x00); return c; }
    static BlanchedAlmond(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xEB, 0xCD); return c; }
    static Blue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0x00, 0xFF); return c; }
    static BlueViolet(): Color { let c = new Color(); c.SetGammaByteRGBA(0x8A, 0x2B, 0xE2); return c; }
    static Brown(): Color { let c = new Color(); c.SetGammaByteRGBA(0xA5, 0x2A, 0x2A); return c; }
    static BurlyWood(): Color { let c = new Color(); c.SetGammaByteRGBA(0xDE, 0xB8, 0x87); return c; }
    static CadetBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x5F, 0x9E, 0xA0); return c; }
    static Chartreuse(): Color { let c = new Color(); c.SetGammaByteRGBA(0x7F, 0xFF, 0x00); return c; }
    static Chocolate(): Color { let c = new Color(); c.SetGammaByteRGBA(0xD2, 0x69, 0x1E); return c; }
    static Coral(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0x7F, 0x50); return c; }
    static CornflowerBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x64, 0x95, 0xED); return c; } // The Original!
    static Cornsilk(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xF8, 0xDC); return c; }
    static Crimson(): Color { let c = new Color(); c.SetGammaByteRGBA(0xDC, 0x14, 0x3C); return c; }
    static Cyan(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0xFF, 0xFF); return c; }
    static DarkBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0x00, 0x8B); return c; }
    static DarkCyan(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0x8B, 0x8B); return c; }
    static DarkGoldenRod(): Color { let c = new Color(); c.SetGammaByteRGBA(0xB8, 0x86, 0x0B); return c; }
    static DarkGray(): Color { let c = new Color(); c.SetGammaByteRGBA(0xA9, 0xA9, 0xA9); return c; }
    static DarkGrey(): Color { let c = new Color(); c.SetGammaByteRGBA(0xA9, 0xA9, 0xA9); return c; }
    static DarkGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0x64, 0x00); return c; }
    static DarkKhaki(): Color { let c = new Color(); c.SetGammaByteRGBA(0xBD, 0xB7, 0x6B); return c; }
    static DarkMagenta(): Color { let c = new Color(); c.SetGammaByteRGBA(0x8B, 0x00, 0x8B); return c; }
    static DarkOliveGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x55, 0x6B, 0x2F); return c; }
    static DarkOrange(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0x8C, 0x00); return c; }
    static DarkOrchid(): Color { let c = new Color(); c.SetGammaByteRGBA(0x99, 0x32, 0xCC); return c; }
    static DarkRed(): Color { let c = new Color(); c.SetGammaByteRGBA(0x8B, 0x00, 0x00); return c; }
    static DarkSalmon(): Color { let c = new Color(); c.SetGammaByteRGBA(0xE9, 0x96, 0x7A); return c; }
    static DarkSeaGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x8F, 0xBC, 0x8F); return c; }
    static DarkSlateBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x48, 0x3D, 0x8B); return c; }
    static DarkSlateGray(): Color { let c = new Color(); c.SetGammaByteRGBA(0x2F, 0x4F, 0x4F); return c; }
    static DarkSlateGrey(): Color { let c = new Color(); c.SetGammaByteRGBA(0x2F, 0x4F, 0x4F); return c; }
    static DarkTurquoise(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0xCE, 0xD1); return c; }
    static DarkViolet(): Color { let c = new Color(); c.SetGammaByteRGBA(0x94, 0x00, 0xD3); return c; }
    static DeepPink(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0x14, 0x93); return c; }
    static DeepSkyBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0xBF, 0xFF); return c; }
    static DimGray(): Color { let c = new Color(); c.SetGammaByteRGBA(0x69, 0x69, 0x69); return c; }
    static DimGrey(): Color { let c = new Color(); c.SetGammaByteRGBA(0x69, 0x69, 0x69); return c; }
    static DodgerBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x1E, 0x90, 0xFF); return c; }
    static FireBrick(): Color { let c = new Color(); c.SetGammaByteRGBA(0xB2, 0x22, 0x22); return c; }
    static FloralWhite(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFA, 0xF0); return c; }
    static ForestGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x22, 0x8B, 0x22); return c; }
    static Fuchsia(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0x00, 0xFF); return c; }
    static Gainsboro(): Color { let c = new Color(); c.SetGammaByteRGBA(0xDC, 0xDC, 0xDC); return c; }
    static GhostWhite(): Color { let c = new Color(); c.SetGammaByteRGBA(0xF8, 0xF8, 0xFF); return c; }
    static Gold(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xD7, 0x00); return c; }
    static GoldenRod(): Color { let c = new Color(); c.SetGammaByteRGBA(0xDA, 0xA5, 0x20); return c; }
    static Gray(): Color { let c = new Color(); c.SetGammaByteRGBA(0x80, 0x80, 0x80); return c; }
    static Grey(): Color { let c = new Color(); c.SetGammaByteRGBA(0x80, 0x80, 0x80); return c; }
    static Green(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0x80, 0x00); return c; }
    static GreenYellow(): Color { let c = new Color(); c.SetGammaByteRGBA(0xAD, 0xFF, 0x2F); return c; }
    static HoneyDew(): Color { let c = new Color(); c.SetGammaByteRGBA(0xF0, 0xFF, 0xF0); return c; }
    static HotPink(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0x69, 0xB4); return c; }
    static IndianRed(): Color { let c = new Color(); c.SetGammaByteRGBA(0xCD, 0x5C, 0x5C); return c; }
    static Indigo(): Color { let c = new Color(); c.SetGammaByteRGBA(0x4B, 0x00, 0x82); return c; }
    static Ivory(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFF, 0xF0); return c; }
    static Khaki(): Color { let c = new Color(); c.SetGammaByteRGBA(0xF0, 0xE6, 0x8C); return c; }
    static Lavender(): Color { let c = new Color(); c.SetGammaByteRGBA(0xE6, 0xE6, 0xFA); return c; }
    static LavenderBlush(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xF0, 0xF5); return c; }
    static LawnGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x7C, 0xFC, 0x00); return c; }
    static LemonChiffon(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFA, 0xCD); return c; }
    static LightBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0xAD, 0xD8, 0xE6); return c; }
    static LightCoral(): Color { let c = new Color(); c.SetGammaByteRGBA(0xF0, 0x80, 0x80); return c; }
    static LightCyan(): Color { let c = new Color(); c.SetGammaByteRGBA(0xE0, 0xFF, 0xFF); return c; }
    static LightGoldenRodYellow(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFA, 0xFA, 0xD2); return c; }
    static LightGray(): Color { let c = new Color(); c.SetGammaByteRGBA(0xD3, 0xD3, 0xD3); return c; }
    static LightGrey(): Color { let c = new Color(); c.SetGammaByteRGBA(0xD3, 0xD3, 0xD3); return c; }
    static LightGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x90, 0xEE, 0x90); return c; }
    static LightPink(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xB6, 0xC1); return c; }
    static LightSalmon(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xA0, 0x7A); return c; }
    static LightSeaGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x20, 0xB2, 0xAA); return c; }
    static LightSkyBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x87, 0xCE, 0xFA); return c; }
    static LightSlateGray(): Color { let c = new Color(); c.SetGammaByteRGBA(0x77, 0x88, 0x99); return c; }
    static LightSlateGrey(): Color { let c = new Color(); c.SetGammaByteRGBA(0x77, 0x88, 0x99); return c; }
    static LightSteelBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0xB0, 0xC4, 0xDE); return c; }
    static LightYellow(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFF, 0xE0); return c; }
    static Lime(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0xFF, 0x00); return c; }
    static LimeGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x32, 0xCD, 0x32); return c; }
    static Linen(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFA, 0xF0, 0xE6); return c; }
    static Magenta(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0x00, 0xFF); return c; }
    static Maroon(): Color { let c = new Color(); c.SetGammaByteRGBA(0x80, 0x00, 0x00); return c; }
    static MediumAquaMarine(): Color { let c = new Color(); c.SetGammaByteRGBA(0x66, 0xCD, 0xAA); return c; }
    static MediumBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0x00, 0xCD); return c; }
    static MediumOrchid(): Color { let c = new Color(); c.SetGammaByteRGBA(0xBA, 0x55, 0xD3); return c; }
    static MediumPurple(): Color { let c = new Color(); c.SetGammaByteRGBA(0x93, 0x70, 0xDB); return c; }
    static MediumSeaGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x3C, 0xB3, 0x71); return c; }
    static MediumSlateBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x7B, 0x68, 0xEE); return c; }
    static MediumSpringGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0xFA, 0x9A); return c; }
    static MediumTurquoise(): Color { let c = new Color(); c.SetGammaByteRGBA(0x48, 0xD1, 0xCC); return c; }
    static MediumVioletRed(): Color { let c = new Color(); c.SetGammaByteRGBA(0xC7, 0x15, 0x85); return c; }
    static MidnightBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x19, 0x19, 0x70); return c; }
    static MintCream(): Color { let c = new Color(); c.SetGammaByteRGBA(0xF5, 0xFF, 0xFA); return c; }
    static MistyRose(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xE4, 0xE1); return c; }
    static Moccasin(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xE4, 0xB5); return c; }
    static NavajoWhite(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xDE, 0xAD); return c; }
    static Navy(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0x00, 0x80); return c; }
    static OldLace(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFD, 0xF5, 0xE6); return c; }
    static Olive(): Color { let c = new Color(); c.SetGammaByteRGBA(0x80, 0x80, 0x00); return c; }
    static OliveDrab(): Color { let c = new Color(); c.SetGammaByteRGBA(0x6B, 0x8E, 0x23); return c; }
    static Orange(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xA5, 0x00); return c; }
    static OrangeRed(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0x45, 0x00); return c; }
    static Orchid(): Color { let c = new Color(); c.SetGammaByteRGBA(0xDA, 0x70, 0xD6); return c; }
    static PaleGoldenRod(): Color { let c = new Color(); c.SetGammaByteRGBA(0xEE, 0xE8, 0xAA); return c; }
    static PaleGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x98, 0xFB, 0x98); return c; }
    static PaleTurquoise(): Color { let c = new Color(); c.SetGammaByteRGBA(0xAF, 0xEE, 0xEE); return c; }
    static PaleVioletRed(): Color { let c = new Color(); c.SetGammaByteRGBA(0xDB, 0x70, 0x93); return c; }
    static PapayaWhip(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xEF, 0xD5); return c; }
    static PeachPuff(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xDA, 0xB9); return c; }
    static Peru(): Color { let c = new Color(); c.SetGammaByteRGBA(0xCD, 0x85, 0x3F); return c; }
    static Pink(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xC0, 0xCB); return c; }
    static Plum(): Color { let c = new Color(); c.SetGammaByteRGBA(0xDD, 0xA0, 0xDD); return c; }
    static PowderBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0xB0, 0xE0, 0xE6); return c; }
    static Purple(): Color { let c = new Color(); c.SetGammaByteRGBA(0x80, 0x00, 0x80); return c; }
    static RebeccaPurple(): Color { let c = new Color(); c.SetGammaByteRGBA(0x66, 0x33, 0x99); return c; }
    static Red(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0x00, 0x00); return c; }
    static RosyBrown(): Color { let c = new Color(); c.SetGammaByteRGBA(0xBC, 0x8F, 0x8F); return c; }
    static RoyalBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x41, 0x69, 0xE1); return c; }
    static SaddleBrown(): Color { let c = new Color(); c.SetGammaByteRGBA(0x8B, 0x45, 0x13); return c; }
    static Salmon(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFA, 0x80, 0x72); return c; }
    static SandyBrown(): Color { let c = new Color(); c.SetGammaByteRGBA(0xF4, 0xA4, 0x60); return c; }
    static SeaGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x2E, 0x8B, 0x57); return c; }
    static SeaShell(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xF5, 0xEE); return c; }
    static Sienna(): Color { let c = new Color(); c.SetGammaByteRGBA(0xA0, 0x52, 0x2D); return c; }
    static Silver(): Color { let c = new Color(); c.SetGammaByteRGBA(0xC0, 0xC0, 0xC0); return c; }
    static SkyBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x87, 0xCE, 0xEB); return c; }
    static SlateBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x6A, 0x5A, 0xCD); return c; }
    static SlateGray(): Color { let c = new Color(); c.SetGammaByteRGBA(0x70, 0x80, 0x90); return c; }
    static SlateGrey(): Color { let c = new Color(); c.SetGammaByteRGBA(0x70, 0x80, 0x90); return c; }
    static Snow(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFA, 0xFA); return c; }
    static SpringGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0xFF, 0x7F); return c; }
    static SteelBlue(): Color { let c = new Color(); c.SetGammaByteRGBA(0x46, 0x82, 0xB4); return c; }
    static Tan(): Color { let c = new Color(); c.SetGammaByteRGBA(0xD2, 0xB4, 0x8C); return c; }
    static Teal(): Color { let c = new Color(); c.SetGammaByteRGBA(0x00, 0x80, 0x80); return c; }
    static Thistle(): Color { let c = new Color(); c.SetGammaByteRGBA(0xD8, 0xBF, 0xD8); return c; }
    static Tomato(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0x63, 0x47); return c; }
    static Turquoise(): Color { let c = new Color(); c.SetGammaByteRGBA(0x40, 0xE0, 0xD0); return c; }
    static Violet(): Color { let c = new Color(); c.SetGammaByteRGBA(0xEE, 0x82, 0xEE); return c; }
    static Wheat(): Color { let c = new Color(); c.SetGammaByteRGBA(0xF5, 0xDE, 0xB3); return c; }
    static White(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFF, 0xFF); return c; }
    static WhiteSmoke(): Color { let c = new Color(); c.SetGammaByteRGBA(0xF5, 0xF5, 0xF5); return c; }
    static Yellow(): Color { let c = new Color(); c.SetGammaByteRGBA(0xFF, 0xFF, 0x00); return c; }
    static YellowGreen(): Color { let c = new Color(); c.SetGammaByteRGBA(0x9A, 0xCD, 0x32); return c; }
}