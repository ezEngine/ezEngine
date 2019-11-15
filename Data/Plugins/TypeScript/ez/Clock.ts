declare function __CPP_Clock_SetSpeed(speed: number): void;
declare function __CPP_Clock_GetSpeed(): number;
declare function __CPP_Clock_GetTimeDiff(): number;
declare function __CPP_Clock_GetAccumulatedTime(): number;

export namespace Clock {

    export function SetClockSpeed(speed: number): void {
        __CPP_Clock_SetSpeed(speed);
    }

    export function GetClockSpeed(): number {
        return __CPP_Clock_GetSpeed();
    }

    export function GetTimeDiff(): number {
        return __CPP_Clock_GetTimeDiff();
    }

    export function GetAccumulatedTime(): number {
        return __CPP_Clock_GetAccumulatedTime();
    }
}
