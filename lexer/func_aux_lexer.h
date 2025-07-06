
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lexer.h"
typedef struct {
    const char* source;
    int pos;
} Lexer;
Lexer* lexer_new(const char* source) ;
void lexer_free(Lexer* lexer);
const char* token_type_to_string(TokenType type);
Token* next_tokens(const char* input, int* count);