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

export namespace Random {
    export function Bool(): boolean {
        return __CPP_RNG_Bool();
    }

    export function DoubleZeroToOneExclusive(): number {
        return __CPP_RNG_DoubleZeroToOneExclusive();
    }

    export function DoubleZeroToOneInclusive(): number {
        return __CPP_RNG_DoubleZeroToOneInclusive();
    }

    export function UIntInRange(range: number): number {
        return __CPP_RNG_UIntInRange(range);
    }

    export function DoubleVarianceAroundZero(variance: number): number {
        return __CPP_RNG_DoubleVarianceAroundZero(variance);
    }

    export function IntInRange(minValue: number, range: number): number {
        return __CPP_RNG_IntInRange(minValue, range);
    }

    export function IntMinMax(minValue: number, maxValue: number): number {
        return __CPP_RNG_IntMinMax(minValue, maxValue);
    }

    export function DoubleInRange(minValue: number, range: number): number {
        return __CPP_RNG_DoubleInRange(minValue, range);
    }

    export function DoubleMinMax(minValue: number, maxValue: number): number {
        return __CPP_RNG_DoubleMinMax(minValue, maxValue);
    }

    export function DoubleVariance(value: number, variance: number): number {
        return __CPP_RNG_DoubleVariance(value, variance);
    }
}
