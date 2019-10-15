export class Time {
    // Converts nanoseconds to Time
    static Nanoseconds(fNanoseconds: number): number {
        return fNanoseconds * 0.000000001;
    }

    // Converts microseconds to Time
    static Microseconds(fMicroseconds: number): number {
        return fMicroseconds * 0.000001;
    }

    // Converts milliseconds to Time
    static Milliseconds(fMilliseconds: number): number {
        return fMilliseconds * 0.001;
    }

    // Converts seconds to Time
    static Seconds(fSeconds: number): number {
        return fSeconds;
    }

    // Converts minutes to Time
    static Minutes(fMinutes: number): number {
        return fMinutes * 60;
    }

    // Converts hours to Time
    static Hours(fHours: number): number {
        return fHours * 60 * 60;
    }

    // Returns a zero Time value
    static Zero(): number {
        return 0;
    }

    // Converts Time to nanoseconds
    static GetNanoseconds(Time: number): number {
        return Time * 1000000000.0;
    }

    // Converts Time to microseconds
    static GetMicroseconds(Time: number): number {
        return Time * 1000000.0;
    }

    // Converts Time to milliseconds
    static GetMilliseconds(Time: number): number {
        return Time * 1000.0;
    }

    // Converts Time to seconds
    static GetSeconds(Time: number): number {
        return Time;
    }

    // Converts Time to minutes
    static GetMinutes(Time: number): number {
        return Time / 60.0;
    }

    // Converts Time to hours
    static GetHours(Time: number): number {
        return Time / (60.0 * 60.0);
    }
}
