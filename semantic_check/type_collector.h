#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h" 
#include "semantic_errors.h" 
#include "../ast_nodes/ast_nodes.h"


typedef struct TypeCollector {
    Context* context;
    HulkErrorList* errors;
} TypeCollector;


// Prototipos de funciones
TypeCollector collect_types(HulkErrorList* errors, ProgramNode* ast);
void TypeCollector_init(TypeCollector* collector, HulkErrorList* errors); //ok
void TypeCollector_visit_program(TypeCollector* collector, ProgramNode* node); //ok
void TypeCollector_visit_type_declaration(TypeCollector* collector, TypeDeclarationNode* node); //ok
void TypeCollector_visit_protocol_declaration(TypeCollector* collector, ProtocolDeclarationNode* node); //ok
void create_hulk_functions(Context* context); //ok
void create_iterable_protocol(Context* context); //ok