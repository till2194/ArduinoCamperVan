#include "LookupTable1D.h"

LookupTable1D::LookupTable1D(const double* inputValues, const double* outputValues, size_t size)
    : inputValues_(inputValues), outputValues_(outputValues), size_(size) {
    // No need to check size since it's the responsibility of the user to provide correct sizes
}

double LookupTable1D::interpolate(double inputValue) const {
    size_t index = findClosestIndex(inputValue);

    // Perform linear interpolation
    double x0 = inputValues_[index];
    double x1 = inputValues_[index + 1];
    double y0 = outputValues_[index];
    double y1 = outputValues_[index + 1];

    // Linear interpolation formula
    return y0 + (y1 - y0) * (inputValue - x0) / (x1 - x0);
}

size_t LookupTable1D::findClosestIndex(double inputValue) const {
    for (size_t i = 0; i < size_ - 1; ++i) {
        if (inputValue >= inputValues_[i] && inputValue <= inputValues_[i + 1]) {
            return i;
        }
    }

    // This should not happen if the input values are sorted
    Serial.println("Error: Failed to find closest index.");
    return 0;
}
