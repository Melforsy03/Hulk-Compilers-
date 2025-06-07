    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <cstdarg>

    // Definición de colores para la terminal (simulando colorama)
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

    char* HulkError_to_string(HulkError* error) {
        if (!error) return NULL;
        
        // Asumiendo que HulkError tiene campos: message, row, column
        char* str = NULL;
        
        if (error->row > 0 && error->column > 0) {
            // Error con posición
            str = format_string("[Error en línea %d, columna %d] %s", 
                            error->row, error->column, error->message);
        } else {
            // Error sin posición
            str = format_string("[Error] %s", error->message);
        }
        
        return str;
    }

    // Implementación de funciones auxiliares para HulkError

    void HulkError_init(HulkError* error, const char* message, int row, int column) {
        if (!error) return;
        
        error->message = message ? strdup(message) : NULL;
        error->row = row;
        error->column = column;
    }

    void HulkError_free(HulkError* error) {
        if (!error) return;
        
        if (error->message) {
            free(error->message);
        }
        
        // Si el error fue asignado dinámicamente
        free(error);
    }

    // Función para crear un nuevo error (versión simplificada)
    HulkError* HulkError_create(const char* message, int row, int column) {
        HulkError* error = (HulkError*)malloc(sizeof(HulkError));
        if (!error) return NULL;
        
        HulkError_init(error, message, row, column);
        return error;
    }
    // Funciones auxiliares para formateo de strings
    
    
    char* format_string(const char* format, ...) {
        va_list args;
        va_start(args, format);
        
        // Determinar el tamaño necesario
        va_list args_copy;
        va_copy(args_copy, args);
        int size = vsnprintf(NULL, 0, format, args_copy) + 1;
        va_end(args_copy);
        
        // Asignar memoria y formatear
        char* buffer = (char*)malloc(size);
        vsnprintf(buffer, size, format, args);
        va_end(args);
        
        return buffer;
    }

    // Implementación de HulkError
    char* HulkError_get_error_type() { return "HulkError"; }

    void HulkError_init(HulkError* error, const char* text, int line, int column) {
        error->text = strdup(text);
        error->line = line;
        error->column = column;
        error->get_error_type = HulkError_get_error_type;
    }

    char* HulkError_to_string(HulkError* error) {
        return format_string("%s: %s", error->get_error_type(), error->text);
    }

    // Implementación de HulkIOError
    char* HulkIOError_get_error_type() { return "IOHulkError"; }

    typedef struct {
        HulkError base;
    } HulkIOError;

    void HulkIOError_init(HulkIOError* error, const char* text) {
        HulkError_init((HulkError*)error, text, -1, -1);
        error->base.get_error_type = HulkIOError_get_error_type;
    }

    // Constantes para HulkIOError
    const char* HULK_IO_INVALID_EXTENSION = "Input file '%s' is not a .hulk file.";
    const char* HULK_IO_ERROR_READING_FILE = "Error reading file '%s'.";
    const char* HULK_IO_ERROR_WRITING_FILE = "Error writing to file '%s'.";

    // Implementación de HulkSyntacticError
    char* HulkSyntacticError_get_error_type() { return "SyntacticError"; }

    typedef struct {
        HulkError base;
    } HulkSyntacticError;

    void HulkSyntacticError_init(HulkSyntacticError* error, const char* text, int line, int column) {
        HulkError_init((HulkError*)error, text, line, column);
        error->base.get_error_type = HulkSyntacticError_get_error_type;
    }

    const char* HULK_SYNTACTIC_MESSAGE = "Error at or near '%s'.";

    // Implementación de HulkLexicographicError
    char* HulkLexicographicError_get_error_type() { return "LEXICOGRAPHIC ERROR"; }

    typedef struct {
        HulkError base;
    } HulkLexicographicError;

    void HulkLexicographicError_init(HulkLexicographicError* error, const char* text, int line, int column) {
        HulkError_init((HulkError*)error, text, line, column);
        error->base.get_error_type = HulkLexicographicError_get_error_type;
    }

    char* HulkLexicographicError_to_string(HulkLexicographicError* error) {
        return format_string(RED "%s: %s -> (line: %d, column: %d)" RESET, 
                            error->base.get_error_type(), 
                            error->base.text, 
                            error->base.line, 
                            error->base.column);
    }

    const char* HULK_LEX_UNKNOWN_TOKEN = "Unknown token '%s'.";
    const char* HULK_LEX_UNTERMINATED_STRING = "Unterminated string '%s'.";

    // Implementación de HulkSemanticError
    char* HulkSemanticError_get_error_type() { return "SEMANTIC ERROR"; }

    typedef struct {
        HulkError base;
    } HulkSemanticError;

    void HulkSemanticError_init(HulkSemanticError* error, const char* text, int line, int column) {
        HulkError_init((HulkError*)error, text, line, column);
        error->base.get_error_type = HulkSemanticError_get_error_type;
    }

    char* HulkSemanticError_to_string(HulkSemanticError* error) {
        return format_string(RED "%s: %s -> (line: %d, column: %d)" RESET, 
                            error->base.get_error_type(), 
                            error->base.text, 
                            error->base.line, 
                            error->base.column);
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

    // Constantes para HulkRunTimeError (muchas son iguales a las semánticas)
    const char* HULK_RT_WRONG_METHOD_RETURN_TYPE = "Method \"%s\" in type \"%s\" has declared return type \"%s\" but returns \"%s\"";
    const char* HULK_RT_WRONG_FUNCTION_RETURN_TYPE = "Function \"%s\" return type is \"%s\" but it returns \"%s\"";
    const char* HULK_RT_VARIABLE_IS_DEFINED = "Variable \"%s\" is already defined in this scope";
    const char* HULK_RT_WRONG_SIGNATURE = "Method \"%s\" already defined in \"%s\" with a different signature.";
    const char* HULK_RT_SELF_IS_READONLY = "Variable \"self\" is read-only.";
    const char* HULK_RT_LOCAL_ALREADY_DEFINED = "Variable \"%s\" is already defined in method \"%s\".";
    const char* HULK_RT_INCOMPATIBLE_TYPES = "Cannot convert \"%s\" into \"%s\".";
    const char* HULK_RT_VARIABLE_NOT_DEFINED = "Variable \"%s\" is not defined in \"%s\".";
    const char* HULK_RT_INVALID_OPERATION = "Operation is not defined between \"%s\" and \"%s\".";
    const char* HULK_RT_INVALID_IS_OPERATION = "Invalid \"IS\" operation: \"%s\" is not a type";
    const char* HULK_RT_INVALID_CAST_OPERATION = "Cast operation is not defined between \"%s\" and \"%s\"";
    const char* HULK_RT_INVALID_UNARY_OPERATION = "Operation is not defined for \"%s\"";
    const char* HULK_RT_VECTOR_OBJECT_DIFFERENT_TYPES = "Vector is conformed by different types";
    const char* HULK_RT_INVALID_INDEXING = "Can not index into a \"%s\"";
    const char* HULK_RT_INVALID_INDEXING_OPERATION = "An index can not be a \"%s\"";
    const char* HULK_RT_NOT_DEFINED = "Variable \"%s\" is not defined";
    const char* HULK_RT_INVALID_TYPE_ARGUMENTS = "Type of param \"%s\" is \"%s\" in \"%s\" but it is being called with type \"%s\" ";
    const char* HULK_RT_INVALID_LEN_ARGUMENTS = "\"%s\" has %s parameters but it is being called with \"%s\" arguments";
    const char* HULK_RT_PRIVATE_ATTRIBUTE = "Cannot access attribute \"%s\" in type \"%s\" is private. All attributes are private";
    const char* HULK_RT_NOT_CONFORMS_TO = "\"%s\" does not conform to \"%s\"";
    const char* HULK_RT_INVALID_OVERRIDE = "Method \"%s\" can not be overridden in class \"%s\".It is already defined in with a different signature.";
    const char* HULK_RT_NOT_DEFINED_PROTOCOL_METHOD_RETURN_TYPE = "Type or Protocol \"%s\" is not defined.";
    const char* HULK_RT_NO_PROTOCOL_RETURN_TYPE = "A return type must me annoted for \"%s\" in Protocol \"%s\"";
    const char* HULK_RT_NO_PROTOCOL_PARAM_TYPE = "A type must be annoted for parameter \"%s\" in method \"%s\" in Protocol \"%s\".";
    const char* HULK_RT_NOT_DEFINED_ATTRIBUTE_TYPE = "Type \"%s\" of attribute \"%s\" in \"%s\" is not defined.";
    const char* HULK_RT_NOT_DEFINED_METHOD_RETURN_TYPE = "Return type \"%s\" of method \"%s\" in \"%s\" is not defined.";
    const char* HULK_RT_NOT_DEFINDED_FUNCTION_RETURN_TYPE = "Return type \"%s\" of function \"%s\" is not defined.";
    const char* HULK_RT_NOT_DEFINED_METHOD_PARAM_TYPE = "Type \"%s\" of parameter\"%s\" in method \"%s\" in \"%s\" is not defined.";
    const char* HULK_RT_NOT_DEFINED_FUNCTION_PARAM_TYPE = "Type \"%s\" of parameter\"%s\" in function \"%s\" is not defined.";
    const char* HULK_RT_NOT_DEFINED_TYPE_CONSTRUCTOR_PARAM_TYPE = "Type \"%s\" of param \"%s\" in type \"%s\" declaration is not defined.";
    const char* HULK_RT_INVALID_INHERITANCE_FROM_DEFAULT_TYPE = "Type \"%s\" can not inherite from Hulk Type \"%s\".";
    const char* HULK_RT_INVALID_CIRCULAR_INHERITANCE = "\"%s\" can not inherite from type \"%s\". Circular inheritance is not allowed.";
    const char* HULK_RT_NOT_DEFINED_PARENT_TYPE = "Type %s of %s 's parent is not defined";
    // Implementación de HulkRunTimeError
    char* HulkRunTimeError_get_error_type() { return "RUN TIME ERROR"; }

    typedef struct {
        HulkError base;
    } HulkRunTimeError;

    void HulkRunTimeError_init(HulkRunTimeError* error, const char* text, int line, int column) {
        HulkError_init((HulkError*)error, text, line, column);
        error->base.get_error_type = HulkRunTimeError_get_error_type;
    }

    char* HulkRunTimeError_to_string(HulkRunTimeError* error) {
        return format_string(RED "%s: %s -> (line: %d, column: %d)" RESET, 
                            error->base.get_error_type(), 
                            error->base.text, 
                            error->base.line, 
                            error->base.column);
    }

    // Función para liberar memoria de un error
    void HulkError_free(HulkError* error) {
        free(error->text);
    }

    // Ejemplo de uso
    int main() {
        HulkLexicographicError lex_error;
        HulkLexicographicError_init(&lex_error, "Token desconocido", 10, 5);
        char* error_str = HulkLexicographicError_to_string(&lex_error);
        printf("%s\n", error_str);
        free(error_str);
        HulkError_free((HulkError*)&lex_error);
        
        return 0;
    }