#include "ConvertQuaternary.hpp"

int convertQuaternary(int inputNum, bool inputType) {
    // Extract 8 digits, padding leading zeros
    int digits[8] = { 0 };
    int temp = inputNum;
    for (int i = 7; i >= 0; --i) {
        digits[i] = temp % 10;
        temp /= 10;
    }

    // Precompute powers of 4 up to 4^9
    long long pow4[10];
    pow4[0] = 1;
    for (int i = 1; i <= 9; ++i) {
        pow4[i] = pow4[i - 1] * 4;
    }

    // Compute decimal value
    long long val;
    if (inputType) {  // 4's complement
        long long uval = 0;
        for (int i = 0; i < 8; ++i) {
            uval += (long long)digits[i] * pow4[7 - i];
        }
        val = uval;
        if (digits[0] >= 2) {
            val -= pow4[8];
        }
    }
    else {  // Negative quaternary
        val = 0;
        long long pwr = 1;
        for (int i = 7; i >= 0; --i) {
            val += (long long)digits[i] * pwr;
            pwr *= -4;
        }
    }

    // Convert to the other representation (9 digits)
    int outd[9] = { 0 };
    if (!inputType) {  // Convert to 4's complement
        long long uval2 = (val >= 0) ? val : pow4[9] + val;
        long long temp_u = uval2;
        for (int i = 8; i >= 0; --i) {
            outd[i] = temp_u % 4;
            temp_u /= 4;
        }
    }
    else {  // Convert to negative quaternary
        long long cval = val;
        int base = -4;
        int abase = 4;
        for (int i = 8; i >= 0; --i) {
            long long rem = cval % base;
            if (rem < 0) rem += abase;
            outd[i] = (int)rem;
            cval = (cval - rem) / base;
        }
    }

    // Build result int, ignoring leading zeros
    int result = 0;
    bool started = false;
    for (int i = 0; i < 9; ++i) {
        if (outd[i] != 0 || started) {
            started = true;
            result = result * 10 + outd[i];
        }
    }
    if (!started) result = 0;

    return result;
}