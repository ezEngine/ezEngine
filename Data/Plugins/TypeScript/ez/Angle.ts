/**
 * Helper functions for working with angles.
 */
export class Angle {

    /** 
     * Converts an angle from degree to radians.
     * 
     * @param degree The angle in degree.
     * @returns The angle in radians.
     */
    static DegreeToRadian(degree: number): number {
        return degree * Math.PI / 180.0;
    }

    /** 
     * Converts an angle from radians to degree.
     * 
     * @param radians The angle in radians.
     * @returns The angle in degree.
     */
    static RadianToDegree(radians: number): number {
        return radians * 180.0 / Math.PI;
    }

    /** 
     * Computes the angle between two angles
     * 
     * @param radianA The first angle in radians.
     * @param radianB The second angle in radians.
     */
    static AngleBetween(radianA: number, radianB: number) {
        return Math.PI - Math.abs(Math.abs(radianA - radianB) - Math.PI);
    }

    /** 
     * Checks whether two angles are approximately equal. Multiples of 2 Pi are not considered to be equal.
     * 
     * @param radianLhs The first angle in radians.
     * @param radianRhs The second angle in radians.
     * @param epsilon Epsilon in radians in which the two angles are considered equal.
     */
    static IsEqualSimple(radianLhs: number, radianRhs: number, epsilon: number): boolean {
        const diff = Angle.AngleBetween(radianLhs, radianRhs);

        return (diff >= -epsilon) && (diff <= epsilon);
    }
}
