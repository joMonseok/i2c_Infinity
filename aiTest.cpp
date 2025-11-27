#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

float compute_led(float right, float soil, float temp) {
    const float b0 = 1124.851052f;
    const float b1 = -0.883796f;
    const float b2 = -1.648485f;
    const float b3 = -2.203546f;

    float led = b0 + b1 * right + b2 * soil + b3 * temp;

    if (led < 0.0f)   led = 0.0f;
    if (led > 1023.0f) led = 1023.0f;

    return led;
}

int main() {
    while (1) {  
        float right = (float)(rand() % 1023);   // 0 ~ 1022
        float soil  = (float)(rand() % 101);    // 0 ~ 100
        float temp  = (float)(rand() % 36);     // 0 ~ 35

        float led_f = compute_led(right, soil, temp);
        int   led   = (int)(led_f + 0.5f);      // 반올림

        printf("right: %7.2f  soil: %6.2f  temp: %6.2f  LED: %4d\n",
               right, soil, temp, led);

        Sleep(1000);
    }

    return 0;
}
