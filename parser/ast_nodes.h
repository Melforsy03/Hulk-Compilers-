// ast_nodes.h
#ifndef AST_NODES_H
#define AST_NODES_H

#include "../grammar/symbol.h"  

typedef enum {
    // Nodo raíz del programa
    NODE_PROGRAM,        // Contiene declaraciones y una expresión principal
    
    // Declaraciones
    NODE_LET,            // Declaración de variable (let)
    NODE_FUNCTION_DEF,   // Declaración de función
    NODE_TYPE_DEF,       // Declaración de tipo
    NODE_INHERITS,       // Herencia entre tipos (no usado directamente)
    NODE_ASSIGN,         // Asignación (=)
    NODE_COLON_ASSIGN,   // Asignación con tipo (:=)
    
    // Estructuras de control
    NODE_IF,             // Condicional if
    NODE_LET_IN,
    NODE_WHILE,          // Bucle while
    NODE_FOR,            // Bucle for
    NODE_RANGE,          // Rango (para bucles)
    NODE_RETURN,         // Retorno de función
    NODE_PRINT,          // Impresión en consola
    
    // Operadores binarios
    NODE_ADD,            // Suma (+)
    NODE_SUB,            // Resta (-)
    NODE_MUL,            // Multiplicación (*)
    NODE_DIV,            // División (/)
    NODE_POW,            // Potenciación (^)
    NODE_MOD,            // Módulo (%)
    NODE_EQ,             // Igualdad (==)
    NODE_NEQ,            // Desigualdad (!=)
    NODE_LT,             // Menor que (<)
    NODE_LTE,            // Menor o igual que (<=)
    NODE_GT,             // Mayor que (>)
    NODE_GTE,            // Mayor o igual que (>=)
    NODE_AND,            // AND lógico (&&)
    NODE_OR,             // OR lógico (||)
    NODE_IS,
    NODE_NOT,            // NOT lógico (!)
    NODE_POSITIVE,
    NODE_NEGATIVE,
    NODE_CONCAT,         // Concatenación de strings (++)
    
    // Llamadas
    NODE_CALL,           // Llamada genérica (obsoleto)
    NODE_CALL_FUNC,      // Llamada a función
    NODE_CALL_METHOD,    // Llamada a método
    NODE_CALL_TYPE_ATTRIBUTE,

    // Accesos
    NODE_ACCESS,         // Acceso a atributo/método
    
    // Literales y átomos
    NODE_VAR,            // Referencia a variable
    NODE_NUMBER,         // Literal numérico
    NODE_STRING,         // Literal de string
    NODE_BOOLEAN,        // Literal booleano
    NODE_SELF,           // Referencia a self
    NODE_BASE,           // Referencia a base
    
    // Estructuras
    NODE_BLOCK,          // Bloque de expresiones
    NODE_TYPE_INSTANTIATION, // Instanciación de tipo
    
    // Vectores
    NODE_EXPLICIT_VECTOR, // Vector explícito [...]
    NODE_IMPLICIT_VECTOR, // Vector implícito [for x in y -> expr]
    
    // Operaciones específicas
    NODE_CAST,           // Casting de tipos
    NODE_CHECK_TYPE,     // Verificación de tipo (is)
    NODE_INDEX           // Indexación (obsoleto, usar NODE_ACCESS)
} NodeType;

//--------------------------------------------Base Node---------------------------------------------
typedef struct Node {
    int row;
    int column;
    void* scope;
    Symbol* symbol; 
    struct Node** children;
    int child_count;
    char* lexeme;
    NodeType tipo ;
} Node;

//--------------------------------------------Depth 1---------------------------------------------

typedef struct ProgramNode {
    Node base;
    void* declarations; // Lista de DeclarationNode*
    void* expression;   // ExpressionNode*
} ProgramNode;

typedef struct DeclarationNode {
    Node base;
} DeclarationNode;

typedef struct ExpressionNode {
    Node base;
} ExpressionNode;

//--------------------------------------------Depth 2---------------------------------------------

//__________________________________________Declarations__________________________________________

typedef struct MethodSignatureNode {
    DeclarationNode base;
    char* name;
    void* params;       // Lista de parámetros
    char* returnType;   // Puede ser NULL
} MethodSignatureNode;

typedef struct MethodDeclarationNode {
    DeclarationNode base;
    char* name;
    void* params;       // Lista de parámetros
    char* returnType;   // Puede ser NULL
    void* body;        // ExpressionNode*
} MethodDeclarationNode;

typedef struct FunctionDeclarationNode {
    DeclarationNode base;
    char* name;
    void* params;       // Lista de parámetros
    char* returnType;   // Puede ser NULL
    void* body;        // ExpressionNode*
} FunctionDeclarationNode;

typedef struct TypeConstructorSignatureNode {
    DeclarationNode base;
    char* name;
    void* params;       // Lista de parámetros (puede estar vacía)
} TypeConstructorSignatureNode;

typedef struct TypeAttributeNode {
    DeclarationNode base;
    char* name;
    void* value;       // ExpressionNode*
    char* type;        // Puede ser NULL
} TypeAttributeNode;

typedef struct TypeDeclarationNode {
    DeclarationNode base;
    char* name;
    void* params;       // Lista de parámetros
    char* parent;       // Por defecto "Object"
    void* parent_args;  // Lista de argumentos para el padre
    void* attributes;   // Lista de TypeAttributeNode*
    void* methods;      // Lista de MethodDeclarationNode*
} TypeDeclarationNode;

typedef struct ProtocolDeclarationNode {
    DeclarationNode base;
    char* name;
    void* methods_signature; // Lista de MethodSignatureNode*
    char* parent;            // Puede ser NULL
} ProtocolDeclarationNode;

typedef struct VarDeclarationNode {
    DeclarationNode base;
    char* name;
    void* value;    // ExpressionNode*
    char* type;     // Puede ser NULL
} VarDeclarationNode;

//__________________________________________Expressions__________________________________________

typedef struct ConditionalNode {
    ExpressionNode base;
    void* default_expre;         // ExpressionNode*
    void* conditions;      // Lista de ExpressionNode*
    void* expressions;     // Lista de ExpressionNode*
} ConditionalNode;

typedef struct LetInNode {
    ExpressionNode base;
    void* variables;       // Lista de VarDeclarationNode*
    void* body;            // ExpressionNode*
} LetInNode;

typedef struct WhileNode {
    ExpressionNode base;
    void* condition;       // ExpressionNode*
    void* body;            // ExpressionNode*
} WhileNode;

typedef struct ForNode {
    ExpressionNode base;
    void* item;            // VarNode*
    void* iterable;        // ExpressionNode*
    void* body;            // ExpressionNode*
} ForNode;

typedef struct DestrNode {
    ExpressionNode base;
    void* var;             // VarNode*
    void* expr;            // ExpressionNode*
} DestrNode;

typedef struct AtomicNode {
    ExpressionNode base;
} AtomicNode;

typedef struct BinaryNode {
    ExpressionNode base;
    void* left;
    void* right;
    char* operator;        // Puede ser NULL
} BinaryNode;

typedef struct UnaryNode {
    ExpressionNode base;
    void* operand;
    char* operator;        // Puede ser NULL
} UnaryNode;

//-----------------------------------------------Depth 3-----------------------------------------------

//________________________________________________Binary_______________________________________________

typedef struct BooleanBinaryNode {
    BinaryNode base;
} BooleanBinaryNode;


typedef struct ComparisonBinaryNode {
    BinaryNode base;
} ComparisonBinaryNode;

typedef struct EqualityBinaryNode {
    BinaryNode base;
} EqualityBinaryNode;

typedef struct StringBinaryNode {
    BinaryNode base;
} StringBinaryNode;

typedef struct ArithmeticBinaryNode {
    BinaryNode base;
} ArithmeticBinaryNode;

typedef struct CheckTypeNode {
    ExpressionNode base;
    void* left;
    void* right;
} CheckTypeNode;

//_________________________________________________Unary_______________________________________________

typedef struct ArithmeticUnaryNode {
    UnaryNode base;
} ArithmeticUnaryNode;

typedef struct ReturnNode {
    Node base; // tipo debe ser NODE_RETURN
    ExpressionNode* expr; // lo que se retorna
} ReturnNode;

typedef struct BooleanUnaryNode {
    UnaryNode base;
} BooleanUnaryNode;

//_________________________________________________Atoms_______________________________________________

typedef struct LiteralNode {
    AtomicNode base;
    char* lex;
} LiteralNode;

typedef struct ExpressionBlockNode {
    AtomicNode base;
    void* expressions;     // Lista de ExpressionNode*
} ExpressionBlockNode;

typedef struct CallFuncNode {
    AtomicNode base;
    char* name;
    void* arguments;       // Lista de argumentos
} CallFuncNode;

typedef struct TypeInstantiationNode {
    AtomicNode base;
    char* name;
    void* arguments;       // Lista de argumentos
} TypeInstantiationNode;

typedef struct ExplicitVectorNode {
    AtomicNode base;
    void* items;           // Lista de items
} ExplicitVectorNode;

typedef struct ImplicitVectorNode {
    AtomicNode base;
    void* expr;            // ExpressionNode*
    void* item;            // VarNode*
    void* iterable;        // ExpressionNode*
} ImplicitVectorNode;

typedef struct IndexObjectNode {
    AtomicNode base;
    void* object;          // ExpressionNode*
    void* pos;             // ExpressionNode*
} IndexObjectNode;

typedef struct CallMethodNode {
    AtomicNode base;
    char* inst_name;
    char* method_name;
    void* method_args;     // Lista de argumentos
} CallMethodNode;

typedef struct CallTypeAttributeNode {
    AtomicNode base;
    char* inst_name;
    char* attribute;
} CallTypeAttributeNode;

typedef struct CastTypeNode {
    AtomicNode base;
    char* inst_name;
    char* type_cast;
} CastTypeNode;

//---------------------------------------------------Depth 4-----------------------------------------------

//___________________________________________________Boolean_______________________________________________

typedef struct OrNode {
    BooleanBinaryNode base;
} OrNode;

typedef struct AndNode {
    BooleanBinaryNode base;
} AndNode;

typedef struct NotNode {
    BooleanUnaryNode base;
} NotNode;

//________________________________________________Comparisons_______________________________________________

typedef struct EqualNode {
    EqualityBinaryNode base;
} EqualNode;

typedef struct NotEqualNode {
    EqualityBinaryNode base;
} NotEqualNode;

typedef struct GreaterNode {
    ComparisonBinaryNode base;
} GreaterNode;

typedef struct GreaterEqualNode {
    ComparisonBinaryNode base;
} GreaterEqualNode;

typedef struct LessNode {
    ComparisonBinaryNode base;
} LessNode;

typedef struct LessEqualNode {
    ComparisonBinaryNode base;
} LessEqualNode;

//___________________________________________________Strings_______________________________________________

typedef struct ConcatNode {
    StringBinaryNode base;
} ConcatNode;

typedef struct DoubleConcatNode {
    StringBinaryNode base;
} DoubleConcatNode;

//__________________________________________________Arithmetic_______________________________________________

typedef struct PlusNode {
    ArithmeticBinaryNode base;
} PlusNode;

typedef struct MinusNode {
    ArithmeticBinaryNode base;
} MinusNode;

typedef struct MultNode {
    ArithmeticBinaryNode base;
} MultNode;

typedef struct DivNode {
    ArithmeticBinaryNode base;
} DivNode;

typedef struct ModNode {
    ArithmeticBinaryNode base;
} ModNode;

typedef struct PowNode {
    ArithmeticBinaryNode base;
} PowNode;

typedef struct PositiveNode {
    ArithmeticUnaryNode base;
} PositiveNode;

typedef struct NegativeNode {
    ArithmeticUnaryNode base;
} NegativeNode;

//__________________________________________________Literals_______________________________________________

typedef struct BooleanNode {
    LiteralNode base;
} BooleanNode;

typedef struct VarNode {
    LiteralNode base;
} VarNode;

typedef struct NumberNode {
    LiteralNode base;
} NumberNode;

typedef struct StringNode {
    LiteralNode base;
} StringNode;

// Constructor y liberación genéricos
Node* create_node(Symbol* symbol, const char* lexeme, int child_count, Node** children);
void print_ast_root(Node* root); // imprime y libera
void free_ast(Node* node);
void print_ast(Node* root);

#endif
