import ez = require("../../TypeScript/ez")

export class Nested extends ez.TickedTypescriptComponent {
    constructor() {
        super()
    }

    /* BEGIN AUTO-GENERATED CODE */
    Number: number = 1;
    Boolean: boolean = true;
    String: string = "Yeah";
    Vec3: ez.Vec3 = new ez.Vec3(1, 2, 3);
    Color: ez.Color = new ez.Color(2.937524, 0.030575, 2.285738, 1);
    /* END AUTO-GENERATED CODE */

    Tick(): void {
        ez.Log.Info("Number: " + this.Number)
        ez.Log.Info("Boolean: " + this.Boolean)
    }

    /*
    Initialize(): void {
    }

    Deinitialize(): void {
    }

    OnActivated(): void {
    }

    OnDeactivated(): void {
    }

    OnSimulationStarted(): void {
    }
    */
}

