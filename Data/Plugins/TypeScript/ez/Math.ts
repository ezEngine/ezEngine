export namespace ezMath {
    export function IsNumberEqual(f1: number, f2: number, epsilon: number): boolean {
        return f1 >= f2 - epsilon && f1 <= f2 + epsilon;
    }

    export function IsNumberZero(f1: number, epsilon: number): boolean {
        return f1 >= -epsilon && f1 <= epsilon;
    }
}
