#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

// Implementación de HulkErrorList
    typedef struct HulkErrorList {
        HulkError** errors;
        int count;
        int capacity;
    } HulkErrorList;
    HulkErrorList* HulkErrorList_create() {
        HulkErrorList* list = (HulkErrorList*)malloc(sizeof(HulkErrorList));
        if (!list) return NULL;
        
        list->capacity = 10; // Capacidad inicial
        list->count = 0;
        list->errors = (HulkError**)malloc(sizeof(HulkError*) * list->capacity);
        
        if (!list->errors) {
            free(list);
            return NULL;
        }
        
        return list;
    }

    void HulkErrorList_add(HulkErrorList* list, HulkError* error) {
        if (!list || !error) return;
        
        // Si necesitamos más espacio
        if (list->count >= list->capacity) {
            int new_capacity = list->capacity * 2;
            HulkError** new_errors = (HulkError**)realloc(list->errors, sizeof(HulkError*) * new_capacity);
            
            if (!new_errors) {
                // Falló el realloc, no podemos agregar el error
                return;
            }
            
            list->errors = new_errors;
            list->capacity = new_capacity;
        }
        
        list->errors[list->count++] = error;
    }

    void HulkErrorList_destroy(HulkErrorList* list) {
        if (!list) return;
        
        // Liberar cada error individual
        for (int i = 0; i < list->count; i++) {
            if (list->errors[i]) {
                HulkError_free(list->errors[i]);
            }
        }
        
        free(list->errors);
        free(list);
    }

// Implementación de HulkSemanticError
char* HulkSemanticError_get_error_type() { return "SEMANTIC ERROR"; }

typedef struct HulkSemanticError {
    HulkError base;
} HulkSemanticError;

void HulkSemanticError_init(HulkSemanticError* error, const char* text, int line, int column) {
    error->base.text = strdup(text);
    error->base.line = line;
    error->base.column = column;
    error->base.get_error_type = HulkSemanticError_get_error_type;
}

char* HulkSemanticError_to_string(HulkSemanticError* error) {
    char* buffer = (char*)malloc(256); // Tamaño suficiente para el mensaje
    snprintf(buffer, 256, RED "%s: %s -> (line: %d, column: %d)" RESET, 
            error->base.get_error_type(), 
            error->base.text, 
            error->base.line, 
            error->base.column);
    return buffer;
}

// Función para liberar memoria de un error
void HulkError_free(HulkError* error) {
    if (error && error->text) {
        free(error->text);
    }
}

// Constantes para HulkSemanticError
const char* HULK_SEM_WRONG_METHOD_RETURN_TYPE = "Method \"%s\" in type \"%s\" has declared return type \"%s\" but returns \"%s\"";
const char* HULK_SEM_WRONG_FUNCTION_RETURN_TYPE = "Function \"%s\" return type is \"%s\" but it returns \"%s\"";
const char* HULK_SEM_VARIABLE_IS_DEFINED = "Variable \"%s\" is already defined in this scope";
const char* HULK_SEM_WRONG_SIGNATURE = "Method \"%s\" already defined in \"%s\" with a different signature.";
const char* HULK_SEM_SELF_IS_READONLY = "Variable \"self\" is read-only.";
const char* HULK_SEM_LOCAL_ALREADY_DEFINED = "Variable \"%s\" is already defined in method \"%s\".";
const char* HULK_SEM_INCOMPATIBLE_TYPES = "Cannot convert \"%s\" into \"%s\".";
const char* HULK_SEM_VARIABLE_NOT_DEFINED = "Variable \"%s\" is not defined in \"%s\".";
const char* HULK_SEM_INVALID_OPERATION = "Operation is not defined between \"%s\" and \"%s\".";
const char* HULK_SEM_INVALID_IS_OPERATION = "Invalid \"IS\" operation: \"%s\" is not a type";
const char* HULK_SEM_INVALID_CAST_OPERATION = "Cast operation is not defined between \"%s\" and \"%s\"";
const char* HULK_SEM_INVALID_UNARY_OPERATION = "Operation is not defined for \"%s\"";
const char* HULK_SEM_VECTOR_OBJECT_DIFFERENT_TYPES = "Vector is conformed by different types";
const char* HULK_SEM_INVALID_INDEXING = "Can not index into a \"%s\"";
const char* HULK_SEM_INVALID_INDEXING_OPERATION = "An index can not be a \"%s\"";
const char* HULK_SEM_NOT_DEFINED = "Variable \"%s\" is not defined";
const char* HULK_SEM_INVALID_TYPE_ARGUMENTS = "Type of param \"%s\" is \"%s\" in \"%s\" but it is being called with type \"%s\" ";
const char* HULK_SEM_INVALID_LEN_ARGUMENTS = "\"%s\" has %s parameters but it is being called with \"%s\" arguments";
const char* HULK_SEM_PRIVATE_ATTRIBUTE = "Cannot access attribute \"%s\" in type \"%s\" is private. All attributes are private";
const char* HULK_SEM_NOT_CONFORMS_TO = "\"%s\" does not conform to \"%s\"";
const char* HULK_SEM_INVALID_OVERRIDE = "Method \"%s\" can not be overridden in class \"%s\".It is already defined in with a different signature.";
const char* HULK_SEM_NOT_DEFINED_PROTOCOL_METHOD_RETURN_TYPE = "Type or Protocol \"%s\" is not defined.";
const char* HULK_SEM_NO_PROTOCOL_RETURN_TYPE = "A return type must me annoted for \"%s\" in Protocol \"%s\"";
const char* HULK_SEM_NO_PROTOCOL_PARAM_TYPE = "A type must be annoted for parameter \"%s\" in method \"%s\" in Protocol \"%s\".";
const char* HULK_SEM_NOT_DEFINED_ATTRIBUTE_TYPE = "Type \"%s\" of attribute \"%s\" in \"%s\" is not defined.";
const char* HULK_SEM_NOT_DEFINED_METHOD_RETURN_TYPE = "Return type \"%s\" of method \"%s\" in \"%s\" is not defined.";
const char* HULK_SEM_NOT_DEFINDED_FUNCTION_RETURN_TYPE = "Return type \"%s\" of function \"%s\" is not defined.";
const char* HULK_SEM_NOT_DEFINED_METHOD_PARAM_TYPE = "Type \"%s\" of parameter\"%s\" in method \"%s\" in \"%s\" is not defined.";
const char* HULK_SEM_NOT_DEFINED_FUNCTION_PARAM_TYPE = "Type \"%s\" of parameter\"%s\" in function \"%s\" is not defined.";
const char* HULK_SEM_NOT_DEFINED_TYPE_CONSTRUCTOR_PARAM_TYPE = "Type \"%s\" of param \"%s\" in type \"%s\" declaration is not defined.";
const char* HULK_SEM_INVALID_INHERITANCE_FROM_DEFAULT_TYPE = "Type \"%s\" can not inherite from Hulk Type \"%s\".";
const char* HULK_SEM_INVALID_CIRCULAR_INHERITANCE = "\"%s\" can not inherite from type \"%s\". Circular inheritance is not allowed.";
const char* HULK_SEM_NOT_DEFINED_PARENT_TYPE = "Type %s of %s 's parent is not defined";

// Ejemplo de uso
int main() {
    HulkSemanticError sem_error;
    HulkSemanticError_init(&sem_error, "Tipo incompatible en operación", 15, 8);
    char* error_str = HulkSemanticError_to_string(&sem_error);
    printf("%s\n", error_str);
    free(error_str);
    HulkError_free((HulkError*)&sem_error);
    
    return 0;
}