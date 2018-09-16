#include <math.h>
#include "util.h"


/* mean
 * vec: pointer to an array containing double values
 * size: number of values (array size)
 */

double mean(double* vec, size_t size) {
    double sum = 0.0;
    for (double* pv = vec; pv < vec + size; pv++) {
        sum += *pv;
    }
    return sum/size;
}

/* standard deviation
 * vec: pointer to an array containing double values
 * size: size of the array
 */
double stdev(double* vec, size_t size) {
    double mn = mean(vec, size);

    double sd = 0.0;
    for (double* pv = vec; pv < vec + size; pv++) {
        sd += pow(*pv - mn, 2);
    }

    return sqrt(sd/(size-1));
}
