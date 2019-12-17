declare function __CPP_Utils_StringToHash(text: string): number;

/**
 * Common utility functions.
 */
export namespace Utils {

    /**
     * Returns whether f1 and f2 are equal within a given epsilon.
     */
    export function IsNumberEqual(f1: number, f2: number, epsilon: number): boolean {
        return f1 >= f2 - epsilon && f1 <= f2 + epsilon;
    }

    /**
     * Checks whether f1 is within epsilon close to zero.
     */
    export function IsNumberZero(f1: number, epsilon: number): boolean {
        return f1 >= -epsilon && f1 <= epsilon;
    }

    /**
     * Computes the hash value for a given string.
     */
    export function StringToHash(text: string): number {
        return __CPP_Utils_StringToHash(text);
    }
}
