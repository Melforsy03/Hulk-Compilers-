#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

#include "semantic.h" 
#include "semantic_errors.h" 
#include "../ast_nodes/ast_nodes.h"


// Estructura del TypeBuilder
typedef struct {
    Context* context;
    Type* current_type;
    HulkErrorList* errors;
} TypeBuilder;

//Prototipos de funciones
void build_types(Context* context, ProgramNode* ast, HulkErrorList* output_errors);
void tb_visit_program(TypeBuilder* builder, ProgramNode* node); //ok
void tb_visit_type_declaration(TypeBuilder* builder, TypeDeclarationNode* node); 
void tb_visit_protocol_declaration(TypeBuilder* builder, ProtocolDeclarationNode* node); 
void tb_visit_function_declaration(TypeBuilder* builder, FunctionDeclarationNode* node); //ok pero revisar conflicto type-protocol
void tb_visit_type_attribute(TypeBuilder* builder, TypeAttributeNode* node); //ok pero revisar conflicto type-protocol
void tb_visit_method_declaration(TypeBuilder* builder, MethodDeclarationNode* node); //ok pero revisar conflicto type-protocol
void tb_visit_method_signature(TypeBuilder* builder, MethodSignatureNode* node); //ok pero revisar conflicto type-protocol
