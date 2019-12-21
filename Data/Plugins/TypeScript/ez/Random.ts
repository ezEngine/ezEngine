declare function __CPP_RNG_Bool(): boolean;
declare function __CPP_RNG_DoubleZeroToOneExclusive(): number;
declare function __CPP_RNG_DoubleZeroToOneInclusive(): number;

declare function __CPP_RNG_UIntInRange(range: number): number;
declare function __CPP_RNG_DoubleVarianceAroundZero(variance: number): number;

declare function __CPP_RNG_IntInRange(minValue: number, range: number): number;
declare function __CPP_RNG_IntMinMax(minValue: number, maxValue: number): number;
declare function __CPP_RNG_DoubleInRange(minValue: number, range: number): number;
declare function __CPP_RNG_DoubleMinMax(minValue: number, maxValue: number): number;
declare function __CPP_RNG_DoubleVariance(value: number, variance: number): number;

/**
 * Functions for generating random numbers.
 */
export namespace Random {

    /**
     * Returns a random boolean value.
     */
    export function Bool(): boolean {
        return __CPP_RNG_Bool();
    }

    /**
     * Returns a number in the range [0; 1).
     */
    export function DoubleZeroToOneExclusive(): number {
        return __CPP_RNG_DoubleZeroToOneExclusive();
    }

    /**
     * Returns a number in the range [0; 1].
     */
    export function DoubleZeroToOneInclusive(): number {
        return __CPP_RNG_DoubleZeroToOneInclusive();
    }

    /**
     * Returns a positive integer number.
     */
    export function UIntInRange(range: number): number {
        return __CPP_RNG_UIntInRange(range);
    }

    /**
     * Returns a double value between [-fAbsMaxValue; +fAbsMaxValue] with a Gaussian distribution.
     */
    export function DoubleVarianceAroundZero(fAbsMaxValue: number): number {
        return __CPP_RNG_DoubleVarianceAroundZero(fAbsMaxValue);
    }

    /**
     * Returns an int32 value in range [minValue ; minValue + range - 1]
     * 
     * A range of 0 is invalid and will assert! It also has no mathematical meaning. A range of 1 already means "between 0 and 1 EXCLUDING 1".
     * So always use a range of at least 1.
     */
    export function IntInRange(minValue: number, range: number): number {
        return __CPP_RNG_IntInRange(minValue, range);
    }

    /**
     * Returns an int32 value in range [minValue ; maxValue]
     */
    export function IntMinMax(minValue: number, maxValue: number): number {
        return __CPP_RNG_IntMinMax(minValue, maxValue);
    }

    /**
     * Returns a number in range [minValue ; minValue + range)
     */
    export function DoubleInRange(minValue: number, range: number): number {
        return __CPP_RNG_DoubleInRange(minValue, range);
    }

    /**
     * Returns a number in range [minValue ; maxValue]
     */
    export function DoubleMinMax(minValue: number, maxValue: number): number {
        return __CPP_RNG_DoubleMinMax(minValue, maxValue);
    }

    /**
     * Returns a number around 'value'' with a given variance (0 - 1 range)
     */
    export function DoubleVariance(value: number, variance: number): number {
        return __CPP_RNG_DoubleVariance(value, variance);
    }
}
