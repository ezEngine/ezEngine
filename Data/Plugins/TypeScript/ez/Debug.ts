import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

import __Color = require("./Color")
export import Color = __Color.Color;

import __Transform = require("./Transform")
export import Transform = __Transform.Transform;

declare function __CPP_Debug_DrawCross(pos: Vec3, size: number, color: Color);
declare function __CPP_Debug_DrawLines(lines: Debug.Line[], color: Color);
declare function __CPP_Debug_DrawLineBox(min: Vec3, max: Vec3, color: Color, transform: Transform);
declare function __CPP_Debug_DrawSolidBox(min: Vec3, max: Vec3, color: Color, transform: Transform)
declare function __CPP_Debug_DrawLineSphere(center: Vec3, radius: number, color: Color, transform: Transform);
declare function __CPP_Debug_Draw3DText(text: string, pos: Vec3, color: Color, sizeInPixel: number);

export namespace Debug {

    export function DrawCross(pos: Vec3, size: number, color: Color) {
        __CPP_Debug_DrawCross(pos, size, color);
    }

    export class Line {
        startX: number = 0;
        startY: number = 0;
        startZ: number = 0;
        endX: number = 0;
        endY: number = 0;
        endZ: number = 0;
    }

    export function DrawLines(lines: Line[], color: Color = null) {
        __CPP_Debug_DrawLines(lines, color);
    }

    export function DrawLineBox(min: Vec3, max: Vec3, color: Color = null, transform: Transform = null) {
        __CPP_Debug_DrawLineBox(min, max, color, transform);
    }

    export function DrawSolidBox(min: Vec3, max: Vec3, color: Color = null, transform: Transform = null) {
        __CPP_Debug_DrawSolidBox(min, max, color, transform);
    }

    export function DrawLineSphere(center: Vec3, radius: number, color: Color = null, transform: Transform = null) {
        __CPP_Debug_DrawLineSphere(center, radius, color, transform);
    }

    export function Draw3DText(text: string, pos: Vec3, color: Color = null, sizeInPixel: number = 16) {
        __CPP_Debug_Draw3DText(text, pos, color, sizeInPixel);
    }

    // DrawLineBoxCorners
    // DrawLineCapsuleZ
    // DrawLineFrustum
}