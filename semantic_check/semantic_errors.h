#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifndef SEMANTIC_ERRORS_H
#define SEMANTIC_ERRORS_H

// Definición de colores para la terminal
#define RED     "\x1B[31m"
#define RESET   "\x1B[0m"


// Estructura base para errores
typedef struct {
    char* text;
    int line;
    int column;
    char* (*get_error_type)(void);
} HulkError;


// Constantes para HulkSemanticError
#define HULK_SEM_WRONG_METHOD_RETURN_TYPE "Method \"%s\" in type \"%s\" has declared return type \"%s\" but returns \"%s\""
#define HULK_SEM_WRONG_FUNCTION_RETURN_TYPE  "Function \"%s\" return type is \"%s\" but it returns \"%s\""
#define HULK_SEM_VARIABLE_IS_DEFINED  "Variable \"%s\" is already defined in this scope"
#define HULK_SEM_WRONG_SIGNATURE  "Method \"%s\" already defined in \"%s\" with a different signature."
#define HULK_SEM_SELF_IS_READONLY  "Variable \"self\" is read-only."
#define HULK_SEM_LOCAL_ALREADY_DEFINED  "Variable \"%s\" is already defined in method \"%s\"."
#define HULK_SEM_INCOMPATIBLE_TYPES  "Cannot convert \"%s\" into \"%s\"."
#define HULK_SEM_VARIABLE_NOT_DEFINED "Variable \"%s\" is not defined in \"%s\"."
#define HULK_SEM_INVALID_OPERATION  "Operation is not defined between \"%s\" and \"%s\"."
#define HULK_SEM_INVALID_IS_OPERATION  "Invalid \"IS\" operation: \"%s\" is not a type"
#define HULK_SEM_INVALID_CAST_OPERATION "Cast operation is not defined between \"%s\" and \"%s\""
#define HULK_SEM_INVALID_UNARY_OPERATION  "Operation is not defined for \"%s\""
#define HULK_SEM_VECTOR_OBJECT_DIFFERENT_TYPES  "Vector is conformed by different types"
#define HULK_SEM_INVALID_INDEXING  "Can not index into a \"%s\""
#define HULK_SEM_INVALID_INDEXING_OPERATION "An index can not be a \"%s\""
#define HULK_SEM_NOT_DEFINED "Variable \"%s\" is not defined"
#define HULK_SEM_INVALID_TYPE_ARGUMENTS "Type of param \"%s\" is \"%s\" in \"%s\" but it is being called with type \"%s\" "
#define HULK_SEM_INVALID_LEN_ARGUMENTS "\"%s\" has %s parameters but it is being called with \"%s\" arguments"
#define HULK_SEM_PRIVATE_ATTRIBUTE "Cannot access attribute \"%s\" in type \"%s\" is private. All attributes are private"
#define HULK_SEM_NOT_CONFORMS_TO "\"%s\" does not conform to \"%s\""
#define HULK_SEM_INVALID_OVERRIDE "Method \"%s\" can not be overridden in class \"%s\".It is already defined in with a different signature."
#define HULK_SEM_NOT_DEFINED_PROTOCOL_METHOD_RETURN_TYPE "Type or Protocol \"%s\" is not defined."
#define HULK_SEM_NO_PROTOCOL_RETURN_TYPE "A return type must me annoted for \"%s\" in Protocol \"%s\""
#define HULK_SEM_NO_PROTOCOL_PARAM_TYPE "A type must be annoted for parameter \"%s\" in method \"%s\" in Protocol \"%s\"."
#define HULK_SEM_NOT_DEFINED_ATTRIBUTE_TYPE "Type \"%s\" of attribute \"%s\" in \"%s\" is not defined."
#define HULK_SEM_NOT_DEFINED_METHOD_RETURN_TYPE "Return type \"%s\" of method \"%s\" in \"%s\" is not defined."
#define HULK_SEM_NOT_DEFINDED_FUNCTION_RETURN_TYPE "Return type \"%s\" of function \"%s\" is not defined."
#define HULK_SEM_NOT_DEFINED_METHOD_PARAM_TYPE "Type \"%s\" of parameter\"%s\" in method \"%s\" in \"%s\" is not defined."
#define HULK_SEM_NOT_DEFINED_FUNCTION_PARAM_TYPE "Type \"%s\" of parameter\"%s\" in function \"%s\" is not defined."
#define HULK_SEM_NOT_DEFINED_TYPE_CONSTRUCTOR_PARAM_TYPE "Type \"%s\" of param \"%s\" in type \"%s\" declaration is not defined."
#define HULK_SEM_INVALID_INHERITANCE_FROM_DEFAULT_TYPE "Type \"%s\" can not inherite from Hulk Type \"%s\"."
#define HULK_SEM_INVALID_CIRCULAR_INHERITANCE "\"%s\" can not inherite from type \"%s\". Circular inheritance is not allowed."
#define HULK_SEM_NOT_DEFINED_PARENT_TYPE  "Type %s of %s 's parent is not defined"


char* HulkSemanticError_get_error_type();

typedef struct HulkSemanticError {
    HulkError base;
} HulkSemanticError;

void HulkSemanticError_init(HulkSemanticError* error, const char* text, int line, int column);

char* HulkSemanticError_to_string(HulkSemanticError* error);

// Función para liberar memoria de un error
void HulkError_free(HulkError* error);


// Implementación de HulkErrorList
    typedef struct HulkErrorList {
        HulkError** errors;
        int count;
        int capacity;
    } HulkErrorList;
    HulkErrorList* HulkErrorList_create();

void HulkErrorList_add(HulkErrorList* list, HulkError* error);

void HulkErrorList_destroy(HulkErrorList* list);
#endif