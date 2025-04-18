
#include <stdio.h>
#include "dfa_converter.h"

void generar_codigo_dfa(const char* nombre_funcion, FILE* f) {
    fprintf(f, "int %s(const char* texto) {\n", nombre_funcion);
    fprintf(f, "    int estado = 0;\n");
    fprintf(f, "    for (int i = 0; texto[i]; i++) {\n");
    fprintf(f, "        char c = texto[i];\n");
    fprintf(f, "        switch (estado) {\n");

    for (int i = 0; i < total_dfa; i++) {
        fprintf(f, "            case %d:\n", i);
        fprintf(f, "                switch (c) {\n");
        for (int c = 32; c < 127; c++) {
            int dest = dfa[i].transiciones[c];
            if (dest != -1) {
                if (c == '\\')
                    fprintf(f, "                    case '\\\\': estado = %d; break;\n", dest);
                else if (c == '\'')
                    fprintf(f, "                    case '\\\'': estado = %d; break;\n", dest);
                else if (c == '\"')
                    fprintf(f, "                    case '\"': estado = %d; break;\n", dest);
                else
                    fprintf(f, "                    case '%c': estado = %d; break;\n", c, dest);
            }
        }
        fprintf(f, "                    default: return 0;\n");
        fprintf(f, "                } break;\n");
    }

    fprintf(f, "            default: return 0;\n");
    fprintf(f, "        }\n    }\n");
    fprintf(f, "    return ");
    for (int i = 0; i < total_dfa; i++) {
        if (dfa[i].es_final) {
            if (i > 0) fprintf(f, " || ");
            fprintf(f, "estado == %d", i);
        }
    }
fprintf(f, ";\n");

    fprintf(f, "}\n");
}
