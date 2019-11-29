declare function __CPP_Utils_StringToHash(text: string): number;

export namespace Utils {
    export function IsNumberEqual(f1: number, f2: number, epsilon: number): boolean {
        return f1 >= f2 - epsilon && f1 <= f2 + epsilon;
    }

    export function IsNumberZero(f1: number, epsilon: number): boolean {
        return f1 >= -epsilon && f1 <= epsilon;
    }

    export function StringToHash(text: string): number {
        return __CPP_Utils_StringToHash(text);
    }
}
