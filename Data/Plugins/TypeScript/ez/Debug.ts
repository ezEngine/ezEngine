import __Vec2 = require("./Vec2")
export import Vec2 = __Vec2.Vec2;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

import __Color = require("./Color")
export import Color = __Color.Color;

import __Transform = require("./Transform")
export import Transform = __Transform.Transform;

declare function __CPP_Debug_DrawCross(pos: Vec3, size: number, color: Color): void;
declare function __CPP_Debug_DrawLines(lines: Debug.Line[], color: Color): void;
declare function __CPP_Debug_DrawLines2D(lines: Debug.Line[], color: Color): void;
declare function __CPP_Debug_DrawLineBox(min: Vec3, max: Vec3, color: Color, transform: Transform): void;
declare function __CPP_Debug_DrawSolidBox(min: Vec3, max: Vec3, color: Color, transform: Transform): void;
declare function __CPP_Debug_DrawLineSphere(center: Vec3, radius: number, color: Color, transform: Transform): void;
declare function __CPP_Debug_Draw2DText(text: string, pos: Vec2, color: Color, sizeInPixel: number, alignHorz: Debug.HorizontalAlignment): void;
declare function __CPP_Debug_Draw3DText(text: string, pos: Vec3, color: Color, sizeInPixel: number): void;
declare function __CPP_Debug_GetResolution(): Vec2;

/**
 * Debug visualization functionality.
 */
export namespace Debug {

    export enum HorizontalAlignment {
        Left,
        Center,
        Right,
    }

    // TODO:
    // DrawLineBoxCorners
    // DrawLineCapsuleZ
    // DrawLineFrustum

    export function GetResolution(): Vec2 {
        return __CPP_Debug_GetResolution();
    }

    /**
     * Renders a cross of three lines at the given position.
     * 
     * @param pos Position in world-space where to render the cross.
     * @param size Length of the cross lines.
     * @param color Color of the cross lines.
     */
    export function DrawCross(pos: Vec3, size: number, color: Color): void {
        __CPP_Debug_DrawCross(pos, size, color);
    }

    /**
     * Represents a line in 3D.
     */
    export class Line {
        startX: number = 0;
        startY: number = 0;
        startZ: number = 0;
        endX: number = 0;
        endY: number = 0;
        endZ: number = 0;
    }

    /**
     * Draws a set of lines with one color in 3D world space.
     */
    export function DrawLines(lines: Line[], color: Color = null): void {
        __CPP_Debug_DrawLines(lines, color);
    }

    /**
     * Draws a set of lines with one color in 2D screen space. Depth (z coordinate) is used for sorting but not for perspective.
     */
    export function DrawLines2D(lines: Line[], color: Color = null): void {
        __CPP_Debug_DrawLines2D(lines, color);
    }

    /**
     * Draws an axis-aligned box out of lines.
     * If the box should not be axis-aligned, the rotation must be set through the transform parameter.
     * 
     * @param min The minimum corner of the AABB.
     * @param max The maximum corner of the AABB.
     * @param color The color of the lines.
     * @param transform Optional trnasformation (rotation, scale, transform) of the bbox.
     */
    export function DrawLineBox(min: Vec3, max: Vec3, color: Color = null, transform: Transform = null): void {
        __CPP_Debug_DrawLineBox(min, max, color, transform);
    }

    /**
     * Draws an axis-aligned solid box.
     * If the box should not be axis-aligned, the rotation must be set through the transform parameter.
     * 
     * @param min The minimum corner of the AABB.
     * @param max The maximum corner of the AABB.
     * @param color The color of the faces.
     * @param transform Optional trnasformation (rotation, scale, transform) of the bbox.
     */
    export function DrawSolidBox(min: Vec3, max: Vec3, color: Color = null, transform: Transform = null): void {
        __CPP_Debug_DrawSolidBox(min, max, color, transform);
    }

    /**
     * Draws a sphere out of lines.
     * 
     * @param center The world-space position of the sphere.
     * @param radius The radius of the sphere.
     * @param color The color of the lines.
     * @param transform An optional transform. Mostly for convenience to just pass in an object's transform, 
     *                  but also makes it possible to squash the sphere with non-uniform scaling.
     */
    export function DrawLineSphere(center: Vec3, radius: number, color: Color = null, transform: Transform = null): void {
        __CPP_Debug_DrawLineSphere(center, radius, color, transform);
    }

    /**
     * Draws text at a pixel position on screen.
     * 
     * @param pos The screen-space position where to render the text.
     * @param sizeInPixel The size of the text in pixels.
     */
    export function Draw2DText(text: string, pos: Vec2, color: Color = null, sizeInPixel: number = 16, alignHorz: HorizontalAlignment = HorizontalAlignment.Left): void {
        __CPP_Debug_Draw2DText(text, pos, color, sizeInPixel, alignHorz);
    }

    /**
     * Draws text at a 3D position, always facing the camera.
     * 
     * @param pos The world-space position where to render the text.
     * @param sizeInPixel The size of the text in pixels. The text will always be the same size and facing the camera.
     */
    export function Draw3DText(text: string, pos: Vec3, color: Color = null, sizeInPixel: number = 16): void {
        __CPP_Debug_Draw3DText(text, pos, color, sizeInPixel);
    }
}