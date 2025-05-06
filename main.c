#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "./codigo_generado/parser.h"
#include "./codigo_generado/codegen.h"

int main() {
    ProgramNode* program = crear_programa();
    generar_programa(program);
    return 0;
}