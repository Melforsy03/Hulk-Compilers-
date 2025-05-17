typedef struct {
    char* message;
    int error_code; // Opcional, para clasificar errores
} SemanticError;

typedef struct {
    SemanticError* error;
    int row;
    int column;
} HulkSemanticError;

HulkSemanticError* hulk_semantic_error_create_str(const char* error_msg, int row, int column);
SemanticError* semantic_error_create(const char* message);
HulkSemanticError* hulk_semantic_error_create(SemanticError* error, int row, int column);
void hulk_semantic_error_free(HulkSemanticError* error);