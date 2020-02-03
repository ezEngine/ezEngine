import ez = require("TypeScript/ez")

export class Prefab extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    NumberVar: number = 11;
    BoolVar: boolean = true;
    StringVar: string = "Hello";
    Vec3Var: ez.Vec3 = new ez.Vec3(1, 2, 3);
    ColorVar: ez.Color = new ez.Color(0.768151, 0.142913, 0.001891, 1);
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

    }
}

