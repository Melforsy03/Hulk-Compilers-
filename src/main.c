#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lexer/lexer.h"

typedef struct {
    const char* source;
    int pos;
} Lexer;
void lexer_free(Lexer* lexer) {
    if (!lexer) return;
    free((char*)lexer->source);  // casteamos porque strdup devuelve char*
    free(lexer);
}
Lexer* lexer_new(const char* source) {
    Lexer* lexer = (Lexer*) malloc(sizeof(Lexer));
    if (!lexer) return NULL;
    lexer->source = strdup(source);  // duplicamos para seguridad
    lexer->pos = 0;
    return lexer;
}
const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_POWER: return "POWER";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_EQUAL_EQUAL: return "EQUAL";
        case TOKEN_NOT_EQUAL: return "NOT_EQUAL";
        case TOKEN_GREATER: return "GREATER";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_LESS: return "LESS";
        case TOKEN_LESS_EQUAL: return "LESS_EQUAL";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_DOT: return "DOT";
        case TOKEN_OR: return "OR";
        case TOKEN_AND: return "AND";
        case TOKEN_NOT: return "NOT";
        case TOKEN_LET: return "LET";
        case TOKEN_IN: return "IN";
        case TOKEN_IF: return "IF";
        case TOKEN_ELIF: return "ELIF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_FOR: return "FOR";
        case TOKEN_FUNCTION: return "FUNCTION";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_TYPE: return "TYPE";
        case TOKEN_NEW: return "NEW";
        case TOKEN_ARROW: return "ARROW";
        case TOKEN_AT: return "AT";
        case TOKEN_EOF: return "EOF";
       

        default: return "UNDEFINED";
    }
}


// Función que lee un archivo entero en memoria
char* leer_archivo(const char* ruta) {
    FILE* archivo = fopen(ruta, "rb");
    if (!archivo) {
        perror("Error al abrir archivo");
        exit(EXIT_FAILURE);
    }

    fseek(archivo, 0, SEEK_END);
    long tam = ftell(archivo);
    rewind(archivo);

    char* buffer = (char*)malloc(tam + 1);
    if (!buffer) {
        perror("Error de memoria");
        fclose(archivo);
        exit(EXIT_FAILURE);
    }

    fread(buffer, 1, tam, archivo);
    buffer[tam] = '\0';
    fclose(archivo);
    return buffer;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s archivo_entrada.txt\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error al abrir el archivo");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char* buffer = (char*) malloc(length + 1);
    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);

    Lexer* lexer = lexer_new(buffer);
    Token token;

    while ((token = next_token(lexer)).type != TOKEN_EOF) {
        print_token(token);
    }
    return 0;
}
