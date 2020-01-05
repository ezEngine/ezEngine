import ez = require("TypeScript/ez")

declare function ezTestFailure(file: string, line: number, func: string, msg: string): void;

function TestFailed(msg: string) {
    let stack = (new Error).stack;
    let str = stack.split("\n")[3];
    let file = str.split(":")[0].split("(")[1];
    file = file.replace(".ts", ".js");
    let line = parseInt(str.split(":")[1]);
    let func = "";

    ezTestFailure(file, line, func, msg);
}

export function BOOL(condition: boolean) {

    if (!condition) {
        TestFailed("");
    }
}

export function FLOAT(f1: number, f2: number, epsilon: number) {

    if (!ez.Utils.IsNumberEqual(f1, f2, epsilon)) {
        TestFailed(f1 + " does not equal " + f2);
    }
}

export function VEC2(v1: ez.Vec2, v2: ez.Vec2, epsilon: number) {

    if (!v1.IsEqual(v2, epsilon)) {
        TestFailed("(" + v1.x + ", " + v1.y + ") does not equal (" + v2.x + ", " + v2.y + ")");
    }
}

export function VEC3(v1: ez.Vec3, v2: ez.Vec3, epsilon: number) {

    if (!v1.IsEqual(v2, epsilon)) {
        TestFailed("(" + v1.x + ", " + v1.y + ", " + v1.z + ") does not equal (" + v2.x + ", " + v2.y + ", " + v2.z + ")");
    }
}

export function QUAT(q1: ez.Quat, q2: ez.Quat, epsilon: number) {

    if (!q1.IsEqualRotation(q2, epsilon)) {
        TestFailed("(" + q1.x + ", " + q1.y + ", " + q1.z + ", " + q1.w + ") does not equal (" + q2.x + ", " + q2.y + ", " + q2.z + ", " + q2.w + ")");
    }
}