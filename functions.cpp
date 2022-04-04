#include "functions.h"

void sort(float measurements[], int count) {
    int didsort = 1;
    float temp = 0;
    while (didsort == 1) {
        didsort = 0;
        for (int i = 1; i < count; i++) {
            if (measurements[i] < measurements[i - 1]) {
                temp = measurements[i - 1];
                measurements[i - 1] = measurements[i];
                measurements[i] = temp;
                didsort = 1;
            }
        }
    }
}

float median(float measurements[], int count) {
    return measurements[count/2];
}
