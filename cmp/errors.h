#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Definiciones de colores (simulando colorama)
#define RED     "\x1B[31m"
#define RESET   "\x1B[0m"

// ------------------------- Estructuras Base -------------------------

typedef struct {
    char* error_type;
    char* text;
    int line;
    int column;
} HulkError;

// ------------------------- Funciones Auxiliares -------------------------

char* format_string(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Determinar el tamaño necesario
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, format, args_copy) + 1;
    va_end(args_copy);
    
    // Asignar memoria y formatear
    char* buffer = malloc(size);
    if (buffer) {
        vsnprintf(buffer, size, format, args);
    }
    
    va_end(args);
    return buffer;
}

HulkError* hulk_error_create(const char* error_type, const char* text, int line, int column) {
    HulkError* error = malloc(sizeof(HulkError));
    if (error) {
        error->error_type = strdup(error_type);
        error->text = strdup(text);
        error->line = line;
        error->column = column;
    }
    return error;
}

void hulk_error_free(HulkError* error) {
    if (error) {
        free(error->error_type);
        free(error->text);
        free(error);
    }
}

char* hulk_error_to_string(HulkError* error) {
    if (!error) return strdup("Unknown error");
    
    if (error->line >= 0 && error->column >= 0) {
        return format_string(RED "%s: %s -> (line: %d, column: %d)" RESET, 
                            error->error_type, error->text, error->line, error->column);
    } else {
        return format_string("%s: %s", error->error_type, error->text);
    }
}

// ------------------------- Errores Específicos -------------------------

// HulkIOError
HulkError* hulk_io_error_create(const char* text, int line, int column) {
    return hulk_error_create("IOHulkError", text, line, column);
}

HulkError* hulk_io_error_invalid_extension(const char* filename) {
    char* text = format_string("Input file '%s' is not a .hulk file.", filename);
    HulkError* error = hulk_io_error_create(text, -1, -1);
    free(text);
    return error;
}

HulkError* hulk_io_error_reading_file(const char* filename) {
    char* text = format_string("Error reading file '%s'.", filename);
    HulkError* error = hulk_io_error_create(text, -1, -1);
    free(text);
    return error;
}

HulkError* hulk_io_error_writing_file(const char* filename) {
    char* text = format_string("Error writing to file '%s'.", filename);
    HulkError* error = hulk_io_error_create(text, -1, -1);
    free(text);
    return error;
}

// HulkSyntacticError
HulkError* hulk_syntactic_error_create(const char* text, int line, int column) {
    return hulk_error_create("SyntacticError", text, line, column);
}

HulkError* hulk_syntactic_error_message(const char* token, int line, int column) {
    char* text = format_string("Error at or near '%s'.", token);
    HulkError* error = hulk_syntactic_error_create(text, line, column);
    free(text);
    return error;
}

// HulkLexicographicError
HulkError* hulk_lexicographic_error_create(const char* text, int line, int column) {
    return hulk_error_create("LEXICOGRAPHIC ERROR", text, line, column);
}

HulkError* hulk_lexicographic_error_unknown_token(const char* token, int line, int column) {
    char* text = format_string("Unknown token '%s'.", token);
    HulkError* error = hulk_lexicographic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_lexicographic_error_unterminated_string(const char* token, int line, int column) {
    char* text = format_string("Unterminated string '%s'.", token);
    HulkError* error = hulk_lexicographic_error_create(text, line, column);
    free(text);
    return error;
}

// HulkSemanticError
HulkError* hulk_semantic_error_create(const char* text, int line, int column) {
    return hulk_error_create("SEMANTIC ERROR", text, line, column);
}

HulkError* hulk_semantic_error_wrong_method_return_type(const char* method, const char* type, 
                                                      const char* declared, const char* actual, 
                                                      int line, int column) {
    char* text = format_string("Method \"%s\" in type \"%s\" has declared return type \"%s\" but returns \"%s\"",
                             method, type, declared, actual);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_wrong_function_return_type(const char* function, 
                                                        const char* declared, const char* actual, 
                                                        int line, int column) {
    char* text = format_string("Function \"%s\" return type is \"%s\" but it returns \"%s\"",
                             function, declared, actual);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_variable_defined(const char* var, int line, int column) {
    char* text = format_string("Variable \"%s\" is already defined in this scope", var);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_wrong_signature(const char* method, const char* type, int line, int column) {
    char* text = format_string("Method \"%s\" already defined in \"%s\" with a different signature.", method, type);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_self_readonly(int line, int column) {
    HulkError* error = hulk_semantic_error_create("Variable \"self\" is read-only.", line, column);
    return error;
}

HulkError* hulk_semantic_error_local_defined(const char* var, const char* method, int line, int column) {
    char* text = format_string("Variable \"%s\" is already defined in method \"%s\".", var, method);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_incompatible_types(const char* from, const char* to, int line, int column) {
    char* text = format_string("Cannot convert \"%s\" into \"%s\".", from, to);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_variable_not_defined(const char* var, const char* scope, int line, int column) {
    char* text = format_string("Variable \"%s\" is not defined in \"%s\".", var, scope);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_invalid_operation(const char* type1, const char* type2, int line, int column) {
    char* text = format_string("Operation is not defined between \"%s\" and \"%s\".", type1, type2);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_invalid_is_operation(const char* type, int line, int column) {
    char* text = format_string("Invalid \"IS\" operation: \"%s\" is not a type", type);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_invalid_cast_operation(const char* from, const char* to, int line, int column) {
    char* text = format_string("Cast operation is not defined between \"%s\" and \"%s\"", from, to);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_invalid_unary_operation(const char* type, int line, int column) {
    char* text = format_string("Operation is not defined for \"%s\"", type);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_vector_different_types(int line, int column) {
    HulkError* error = hulk_semantic_error_create("Vector is conformed by different types", line, column);
    return error;
}

HulkError* hulk_semantic_error_invalid_indexing(const char* type, int line, int column) {
    char* text = format_string("Can not index into a \"%s\"", type);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_invalid_indexing_type(const char* type, int line, int column) {
    char* text = format_string("An index can not be a \"%s\"", type);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_not_defined(const char* var, int line, int column) {
    char* text = format_string("Variable \"%s\" is not defined", var);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_invalid_type_args(const char* param, const char* expected, 
                                               const char* context, const char* actual, 
                                               int line, int column) {
    char* text = format_string("Type of param \"%s\" is \"%s\" in \"%s\" but it is being called with type \"%s\"",
                             param, expected, context, actual);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_invalid_args_length(const char* name, int expected, 
                                                 int actual, int line, int column) {
    char* text = format_string("\"%s\" has %d parameters but it is being called with %d arguments",
                             name, expected, actual);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_private_attribute(const char* attr, const char* type, int line, int column) {
    char* text = format_string("Cannot access attribute \"%s\" in type \"%s\" is private. All attributes are private",
                             attr, type);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_not_conforms_to(const char* type, const char* protocol, int line, int column) {
    char* text = format_string("\"%s\" does not conform to \"%s\"", type, protocol);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_invalid_override(const char* method, const char* type, int line, int column) {
    char* text = format_string("Method \"%s\" can not be overridden in class \"%s\". It is already defined with a different signature.",
                             method, type);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_undefined_protocol_method_type(const char* type, int line, int column) {
    char* text = format_string("Type or Protocol \"%s\" is not defined.", type);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_no_protocol_return_type(const char* method, const char* protocol, int line, int column) {
    char* text = format_string("A return type must be annoted for \"%s\" in Protocol \"%s\"", method, protocol);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_no_protocol_param_type(const char* param, const char* method, 
                                                     const char* protocol, int line, int column) {
    char* text = format_string("A type must be annoted for parameter \"%s\" in method \"%s\" in Protocol \"%s\".",
                             param, method, protocol);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_undefined_attribute_type(const char* type, const char* attr, 
                                                       const char* context, int line, int column) {
    char* text = format_string("Type \"%s\" of attribute \"%s\" in \"%s\" is not defined.", type, attr, context);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_undefined_method_return_type(const char* type, const char* method, 
                                                          const char* context, int line, int column) {
    char* text = format_string("Return type \"%s\" of method \"%s\" in \"%s\" is not defined.", type, method, context);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_undefined_function_return_type(const char* type, const char* function, 
                                                            int line, int column) {
    char* text = format_string("Return type \"%s\" of function \"%s\" is not defined.", type, function);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_undefined_method_param_type(const char* type, const char* param, 
                                                         const char* method, const char* context, 
                                                         int line, int column) {
    char* text = format_string("Type \"%s\" of parameter \"%s\" in method \"%s\" in \"%s\" is not defined.",
                             type, param, method, context);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_undefined_function_param_type(const char* type, const char* param, 
                                                           const char* function, int line, int column) {
    char* text = format_string("Type \"%s\" of parameter \"%s\" in function \"%s\" is not defined.",
                             type, param, function);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_undefined_type_constructor_param_type(const char* type, const char* param, 
                                                                    const char* context, int line, int column) {
    char* text = format_string("Type \"%s\" of param \"%s\" in type \"%s\" declaration is not defined.",
                             type, param, context);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_invalid_inheritance(const char* type, const char* parent, int line, int column) {
    char* text = format_string("Type \"%s\" can not inherite from Hulk Type \"%s\".", type, parent);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_circular_inheritance(const char* type, const char* parent, int line, int column) {
    char* text = format_string("\"%s\" can not inherite from type \"%s\". Circular inheritance is not allowed.",
                             type, parent);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_semantic_error_undefined_parent_type(const char* type, const char* context, int line, int column) {
    char* text = format_string("Type %s of %s's parent is not defined", type, context);
    HulkError* error = hulk_semantic_error_create(text, line, column);
    free(text);
    return error;
}

// ------------------------- HulkRunTimeError -------------------------

HulkError* hulk_runtime_error_create(const char* text, int line, int column) {
    return hulk_error_create("RUN TIME ERROR", text, line, column);
}

// Implementaciones específicas de runtime error (similares a las semantic errors pero con diferente tipo)

HulkError* hulk_runtime_error_wrong_method_return_type(const char* method, const char* type, 
                                                      const char* declared, const char* actual, 
                                                      int line, int column) {
    char* text = format_string("Method \"%s\" in type \"%s\" has declared return type \"%s\" but returns \"%s\"",
                             method, type, declared, actual);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_wrong_function_return_type(const char* function, 
                                                        const char* declared, const char* actual, 
                                                        int line, int column) {
    char* text = format_string("Function \"%s\" return type is \"%s\" but it returns \"%s\"",
                             function, declared, actual);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_variable_defined(const char* var, int line, int column) {
    char* text = format_string("Variable \"%s\" is already defined in this scope", var);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_wrong_signature(const char* method, const char* type, int line, int column) {
    char* text = format_string("Method \"%s\" already defined in \"%s\" with a different signature.", method, type);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_self_readonly(int line, int column) {
    HulkError* error = hulk_runtime_error_create("Variable \"self\" is read-only.", line, column);
    return error;
}

HulkError* hulk_runtime_error_local_defined(const char* var, const char* method, int line, int column) {
    char* text = format_string("Variable \"%s\" is already defined in method \"%s\".", var, method);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_incompatible_types(const char* from, const char* to, int line, int column) {
    char* text = format_string("Cannot convert \"%s\" into \"%s\".", from, to);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_variable_not_defined(const char* var, const char* scope, int line, int column) {
    char* text = format_string("Variable \"%s\" is not defined in \"%s\".", var, scope);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_invalid_operation(const char* type1, const char* type2, int line, int column) {
    char* text = format_string("Operation is not defined between \"%s\" and \"%s\".", type1, type2);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_invalid_is_operation(const char* type, int line, int column) {
    char* text = format_string("Invalid \"IS\" operation: \"%s\" is not a type", type);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_invalid_cast_operation(const char* from, const char* to, int line, int column) {
    char* text = format_string("Cast operation is not defined between \"%s\" and \"%s\"", from, to);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_invalid_unary_operation(const char* type, int line, int column) {
    char* text = format_string("Operation is not defined for \"%s\"", type);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_vector_different_types(int line, int column) {
    HulkError* error = hulk_runtime_error_create("Vector is conformed by different types", line, column);
    return error;
}

HulkError* hulk_runtime_error_invalid_indexing(const char* type, int line, int column) {
    char* text = format_string("Can not index into a \"%s\"", type);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_invalid_indexing_type(const char* type, int line, int column) {
    char* text = format_string("An index can not be a \"%s\"", type);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_not_defined(const char* var, int line, int column) {
    char* text = format_string("Variable \"%s\" is not defined", var);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_invalid_type_args(const char* param, const char* expected, 
                                               const char* context, const char* actual, 
                                               int line, int column) {
    char* text = format_string("Type of param \"%s\" is \"%s\" in \"%s\" but it is being called with type \"%s\"",
                             param, expected, context, actual);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_invalid_args_length(const char* name, int expected, 
                                                 int actual, int line, int column) {
    char* text = format_string("\"%s\" has %d parameters but it is being called with %d arguments",
                             name, expected, actual);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_private_attribute(const char* attr, const char* type, int line, int column) {
    char* text = format_string("Cannot access attribute \"%s\" in type \"%s\" is private. All attributes are private",
                             attr, type);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_not_conforms_to(const char* type, const char* protocol, int line, int column) {
    char* text = format_string("\"%s\" does not conform to \"%s\"", type, protocol);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

HulkError* hulk_runtime_error_invalid_override(const char* method, const char* type, int line, int column) {
    char* text = format_string("Method \"%s\" can not be overridden in class \"%s\". It is already defined with a different signature.",
                             method, type);
    HulkError* error = hulk_runtime_error_create(text, line, column);
    free(text);
    return error;
}

