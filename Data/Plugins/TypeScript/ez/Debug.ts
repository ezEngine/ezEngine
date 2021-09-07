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

import __Component = require("./Component")

declare function __CPP_Debug_DrawCross(pos: Vec3, size: number, color: Color, transform: Transform): void;
declare function __CPP_Debug_DrawLines(lines: Debug.Line[], color: Color): void;
declare function __CPP_Debug_Draw2DLines(lines: Debug.Line[], color: Color): void;
declare function __CPP_Debug_DrawLineBox(min: Vec3, max: Vec3, color: Color, transform: Transform): void;
declare function __CPP_Debug_DrawSolidBox(min: Vec3, max: Vec3, color: Color, transform: Transform): void;
declare function __CPP_Debug_DrawLineSphere(center: Vec3, radius: number, color: Color, transform: Transform): void;
declare function __CPP_Debug_Draw2DText(text: string, pos: Vec2, color: Color, sizeInPixel: number, alignHorz: Debug.HorizontalAlignment): void;
declare function __CPP_Debug_Draw3DText(text: string, pos: Vec3, color: Color, sizeInPixel: number): void;
declare function __CPP_Debug_DrawInfoText(corner: Debug.ScreenPlacement, text: string, color: Color): void;
declare function __CPP_Debug_GetResolution(): Vec2;
declare function __CPP_Debug_ReadCVarBool(name: string): boolean;
declare function __CPP_Debug_ReadCVarInt(name: string): number;
declare function __CPP_Debug_ReadCVarFloat(name: string): number;
declare function __CPP_Debug_ReadCVarString(name: string): string;
declare function __CPP_Debug_WriteCVarBool(name: string, value: boolean): void;
declare function __CPP_Debug_WriteCVarInt(name: string, value: number): void;
declare function __CPP_Debug_WriteCVarFloat(name: string, value: number): void;
declare function __CPP_Debug_WriteCVarString(name: string, value: string): void;
declare function __CPP_Debug_RegisterCVar(name: string, type: number, defaultValue: any, description: string): void;
declare function __CPP_Debug_RegisterCFunc(owner: __Component.TypescriptComponent, funcName: string, funcDesc: string, func: any, ...argTypes: Debug.ArgType[]): void;


/**
 * Debug visualization functionality.
 */
export namespace Debug {

    export enum HorizontalAlignment {
        Left,
        Center,
        Right,
    }

    export enum ScreenPlacement {
        TopLeft,
        TopCenter,
        TopRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
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
     * @param transform Optional transformation (rotation, scale, translation) of the cross.
     */
    export function DrawCross(pos: Vec3, size: number, color: Color, transform: Transform = null): void {
        __CPP_Debug_DrawCross(pos, size, color, transform);
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
    export function Draw2DLines(lines: Line[], color: Color = null): void {
        __CPP_Debug_Draw2DLines(lines, color);
    }

    /**
     * Draws an axis-aligned box out of lines.
     * If the box should not be axis-aligned, the rotation must be set through the transform parameter.
     * 
     * @param min The minimum corner of the AABB.
     * @param max The maximum corner of the AABB.
     * @param color The color of the lines.
     * @param transform Optional transformation (rotation, scale, translation) of the bbox.
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
     * @param transform Optional transformation (rotation, scale, translation) of the bbox.
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
     * The string may contain newlines (\n) for multi-line output.
     * If horizontal alignment is right, the entire text block is aligned according to the longest line.
     * 
     * Data can be output as a table, by separating columns with tabs (\n). For example:\n
     * "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|"
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

    /**
     * Draws text in one of the screen corners.
     * Makes sure text in the same corner does not overlap.
     * Has the same formatting options as Draw2DText().
     * 
     * @param corner In which area of the screen to position the text.
     */
    export function DrawInfoText(corner: Debug.ScreenPlacement, text: string, color: Color = null): void {
        __CPP_Debug_DrawInfoText(corner, text, color);
    }

    /**
     * Reads the boolean CVar of the given name and returns its value.
     * If no CVar with this name exists or it uses a different type, 'undefined' is returned.
     */
    export function ReadCVar_Boolean(name: string): boolean { // [tested]
        return __CPP_Debug_ReadCVarBool(name);
    }

    /**
     * Reads the boolean CVar of the given name and returns its value.
     * If no CVar with this name exists or it uses a different type, 'undefined' is returned.
     */
    export function ReadCVar_Int(name: string): number { // [tested]
        return __CPP_Debug_ReadCVarInt(name);
    }

    /**
     * Reads the boolean CVar of the given name and returns its value.
     * If no CVar with this name exists or it uses a different type, 'undefined' is returned.
     */
    export function ReadCVar_Float(name: string): number { // [tested]
        return __CPP_Debug_ReadCVarFloat(name);
    }

    /**
     * Reads the boolean CVar of the given name and returns its value.
     * If no CVar with this name exists or it uses a different type, 'undefined' is returned.
     */
    export function ReadCVar_String(name: string): string { // [tested]
        return __CPP_Debug_ReadCVarString(name);
    }

    /**
     * Stores the given value in the CVar with the provided name.
     * Throws an error if no such CVar exists or the existing one is not of the expected type.
     */
    export function WriteCVar_Boolean(name: string, value: boolean): void { // [tested]
        return __CPP_Debug_WriteCVarBool(name, value);
    }

    /**
     * Stores the given value in the CVar with the provided name.
     * Throws an error if no such CVar exists or the existing one is not of the expected type.
     */
    export function WriteCVar_Int(name: string, value: number): void { // [tested]
        return __CPP_Debug_WriteCVarInt(name, value);
    }

    /**
     * Stores the given value in the CVar with the provided name.
     * Throws an error if no such CVar exists or the existing one is not of the expected type.
     */
    export function WriteCVar_Float(name: string, value: number): void { // [tested]
        return __CPP_Debug_WriteCVarFloat(name, value);
    }

    /**
     * Stores the given value in the CVar with the provided name.
     * Throws an error if no such CVar exists or the existing one is not of the expected type.
     */
    export function WriteCVar_String(name: string, value: string): void { // [tested]
        return __CPP_Debug_WriteCVarString(name, value);
    }

    /**
     * Creates a new CVar with the given name, value and description.
     * If a CVar with this name was already created before, the call is ignored.
     * The CVar can be modified like any other CVar and thus allows external configuration of the script code.
     * When the world in which this script is executed is destroyed, the CVar will cease existing as well.
     */
    export function RegisterCVar_Int(name: string, value: number, description: string): void { // [tested]
        __CPP_Debug_RegisterCVar(name, 0, value, description);
    }

    /**
     * See RegisterCVar_Int
     */
    export function RegisterCVar_Float(name: string, value: number, description: string): void { // [tested]
        __CPP_Debug_RegisterCVar(name, 1, value, description);
    }

    /**
     * See RegisterCVar_Int
     */
    export function RegisterCVar_Boolean(name: string, value: boolean, description: string): void { // [tested]
        __CPP_Debug_RegisterCVar(name, 2, value, description);
    }

    /**
     * See RegisterCVar_Int
     */
    export function RegisterCVar_String(name: string, value: string, description: string): void { // [tested]
        __CPP_Debug_RegisterCVar(name, 3, value, description);
    }

    export enum ArgType { // see ezVariant::Type for values
        Boolean = 2,
        Number = 12,
        String = 27,
    }

    /**
     * Registers a function as a console function.
     * The function can be registered multiple times with different 'func' arguments, to bind the call to multiple objects,
     * however, the list of argument types must be identical each time.
     * 
     * @param owner The component that owns this function. If the component dies, the function will not be called anymore.
     * @param funcName The name under which to expose the function. E.g. "Print"
     * @param funcDesc A description of the function. Should ideally begin with the argument list to call it by. E.g.: "(text: string): Prints 'text' on screen."
     * @param func The typescript function to execute. Must accept the arguments as described by 'argTypes'. E.g. "function Print(text: string)".
     * @param argTypes Variadic list describing the type of each argument. E.g. "ez.Debug.ArgType.String, ez.Debug.ArgType.Number"
     */
    export function RegisterConsoleFunc(owner: __Component.TypescriptComponent, funcName: string, funcDesc: string, func: any, ...argTypes: Debug.ArgType[]): void { // [tested]
        __CPP_Debug_RegisterCFunc(owner, funcName, funcDesc, func, ...argTypes);
    }

}