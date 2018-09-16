//
// Created by Claudio Alvarez Gomez on 8/16/18.
//

#ifndef T1_1_UTIL_H
#define T1_1_UTIL_H
#include <unistd.h>
#include <stdio.h>
#include <math.h>

/* mean
 * vec: pointer to an array containing double values
 * size: number of values (array size)
 */
double mean(double*, size_t);

/* standard deviation
 * vec: pointer to an array containing double values
 * size: size of the array
 */
double stdev(double* vec, size_t size);

#endif //T1_1_UTIL_H
