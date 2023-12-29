#ifndef LOOKUPTABLE1D_H
#define LOOKUPTABLE1D_H

#include <Arduino.h>

class LookupTable1D {
public:
    LookupTable1D(const double* inputValues, const double* outputValues, size_t size);
    double interpolate(double inputValue) const;

private:
    size_t findClosestIndex(double inputValue) const;

private:
    const double* inputValues_;
    const double* outputValues_;
    size_t size_;
};

#endif // LOOKUPTABLE1D_H
