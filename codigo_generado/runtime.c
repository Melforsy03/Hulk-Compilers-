#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
char* bool_to_string(int b) {
    return strdup(b ? "true" : "false");
}

char* float_to_string(float f) {
    char* buf = malloc(32);
    snprintf(buf, 32, "%.2f", f);
    return buf;
}
