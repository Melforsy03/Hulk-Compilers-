#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
void print_int(int x) {
    printf("%d\n", x);
}
void print_str(char* s) {
    printf("%s\n", s);
}
char* int_to_string(int val) {
    char* buffer = malloc(12);
    snprintf(buffer, 12, "%d", val);
    return buffer;
}
char* strcat2(const char* a, const char* b) {
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    char* result = malloc(len_a + len_b + 1);
    strcpy(result, a);
    strcat(result, b);
    return result;
}
int int_pow(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; i++)
        result *= base;
    return result;
}
char* float_to_string(float x) {
    char* buffer = malloc(32);
    snprintf(buffer, 32, "%.6f", x);
    return buffer;
}
char* bool_to_string(int b) {
    return b ? "true" : "false";
}
void print_float(float x) {
    printf("%f\n", x);
}
// Funciones trigonométricas (ya existen en math.h, pero puedes envolverlas)
float my_sin(float x) { return sinf(x); }
float my_cos(float x) { return cosf(x); }
float my_tan(float x) { return tanf(x); }

// Cotangente (no está en math.h)
float my_cot(float x) { return 1.0f / tanf(x); }