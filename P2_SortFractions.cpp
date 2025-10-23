#include "SortFractions.hpp"

// Helper function to compare two fractions, a and b.
// Returns true if a should come before b, false otherwise.
bool compareFractions(int* a, int* b) {
    long long n_a = a[0];
    long long d_a = a[1];
    long long n_b = b[0];
    long long d_b = b[1];

    // Rule 2: If two fractions have the same value, the one with the smaller numerator comes first.
    if (n_a * d_b == n_b * d_a) {
        return n_a < n_b;
    }

    // Rule 1: Sorting based on Cantor's snake path.
    long long sum_a = n_a + d_a;
    long long sum_b = n_b + d_b;

    if (sum_a != sum_b) {
        return sum_a < sum_b;
    }

    // If sums are equal, they are on the same diagonal.
    // If the sum is even, the path is down-left (numerator increases).
    // The one with the smaller numerator comes first.
    if (sum_a % 2 == 0) {
        return n_a < n_b;
    } 
    // If the sum is odd, the path is up-right (numerator decreases).
    // The one with the larger numerator comes first.
    else {
        return n_a > n_b;
    }
}


void bubbleSortFractions(int** fracList, int listSize) {
    // Keep track of the original pointers to set the backtracking pointers later.
    int** original_locations = new int*[listSize];
    for (int i = 0; i < listSize; ++i) {
        original_locations[i] = fracList[2 * i];
    }
    
    // Non-optimized vanilla bubble sort
    for (int i = 0; i < listSize - 1; ++i) {
        for (int j = 0; j < listSize - i - 1; ++j) {
            // If the fraction at j+1 should come before the one at j, swap them.
            if (compareFractions(fracList[2 * (j + 1)], fracList[2 * j])) {
                // Swap the pointers to the fraction data
                int* temp = fracList[2 * j];
                fracList[2 * j] = fracList[2 * (j + 1)];
                fracList[2 * (j + 1)] = temp;
                
                // Increment flipCount for both fractions
                fracList[2 * j][2]++;
                fracList[2 * (j + 1)][2]++;
            }
        }
    }

    // Set the backtracking pointers.
    // For each final position 'k', find which original fraction is now there.
    for (int k = 0; k < listSize; ++k) {
        int* current_fraction_ptr = fracList[2 * k];
        // Find the original index of this fraction
        for (int orig_idx = 0; orig_idx < listSize; ++orig_idx) {
            if (original_locations[orig_idx] == current_fraction_ptr) {
                // The fraction that was originally at 'orig_idx' is now at 'k'.
                // Set the pointer in the original slot to point to the new location.
                fracList[2 * orig_idx + 1] = (int*)&fracList[2 * k];
                break;
            }
        }
    }

    delete[] original_locations;
}