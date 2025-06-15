#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "semantic_errors.h"



// Implementación de HulkSemanticError
char* HulkSemanticError_get_error_type() { return "SEMANTIC ERROR"; }

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