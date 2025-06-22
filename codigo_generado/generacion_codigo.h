#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast_nodes.h"
#include <stdio.h>
#define MAX_TIPOS 50
#define MAX_CAMPOS 20
// ===== Tipos de datos y estructuras =====
typedef enum { TIPO_INT, TIPO_FLOAT, TIPO_VOID ,TIPO_STRING ,TIPO_BOOL ,TIPO_DESCONOCIDO } LLVMType;
typedef struct {
    char* nombre;
    LLVMType tipo_retorno;
    LLVMType tipos_param[10];  // soporta hasta 10 parámetros
    int cantidad_param;
} EntradaFuncion;

typedef struct {
    char* valor;
    int id;
} StringConst;

typedef enum {
    VAR_TYPE_INT,
    VAR_TYPE_STRING ,
    VAR_TYPE_FLOAT,
    VAR_TYPE_BOOL
} VarType;

typedef struct {
    char* nombre;
    VarType tipo;
    int inicializada;
} Variable;

typedef struct {
    char* nombre;
    char* valor;  
} Constante;


typedef struct {
    char* nombre;                     // Nombre del tipo (ej. "Persona")
    int num_campos;
    char* nombres_campos[MAX_CAMPOS]; // Ej. {"nombre", "edad"}
    LLVMType tipos_campos[MAX_CAMPOS]; // Ej. {TIPO_STRING, TIPO_INT}
} RegistroTipo;

typedef struct {
    Node base;
    char* tipo_nombre;              // Nombre del tipo (ej: "Persona")
    ExpressionNode** valores;      // Valores para los campos, en orden
} StructInitNode;

typedef struct {
    Node base;
    void* inst;        // nodo de la instancia (ej: VarNode "p")
    char* inst_name;   // nombre del tipo (ej: "Persona")
    char* campo;       // campo a modificar (ej: "edad")
    ExpressionNode* valor;
} StructAssignNode;

extern RegistroTipo tipos[MAX_TIPOS];
extern int total_tipos;

// ===== Constantes =====

#define MAX_VARIABLES 100
#define MAX_STRING_CONSTS 100
#define MAX_CONST_ENV 100
#define MAX_FUNCIONES 100
// ===== Variables globales =====


extern EntradaFuncion funciones[100];

extern int total_funciones;
extern Variable variables_usadas[MAX_VARIABLES];
extern int num_variables;

extern StringConst constantes_string[MAX_STRING_CONSTS];
extern int num_strings;

extern Constante entorno_constantes[MAX_CONST_ENV];
extern int num_constantes;

extern FILE* salida_llvm;
LLVMType tipo_funcion(const char* nombre);
void registrar_funcion(const char* nombre, LLVMType tipo);
int obtener_indice_campo(const char* tipo_nombre, const char* campo);
int generar_codigo(ExpressionNode* expr);
void generar_programa(ProgramNode* program);
void generar_funciones(ExpressionNode* expr);
LLVMType obtener_tipo_campo(const char* tipo_nombre, const char* campo);
// ===== Temporales y etiquetas =====

int nuevo_temp();
int nuevo_label();
void reset_temporales();
const char* nuevo_tmp();
const char* tmp_actual();

// ===== Variables y entorno =====

const char* obtener_nombre_variable(VarNode* var);
VarType obtener_tipo_variable(const char* nombre);
void registrar_variables(ProgramNode* program);
int variable_existe(const char* nombre);

// ===== Optimizaciones =====

ExpressionNode* optimizar_constantes(ExpressionNode* expr);
ExpressionNode* crear_nodo_constante(const char* valor);
const char* obtener_valor_constante(const char* nombre);

// ===== Funciones auxiliares =====
char* nombre_llvm(const char* nombre);
int get_valor(ExpressionNode* node);
int generar_binario(BinaryNode* bin, const char* opcode);
int generar_comparacion(BinaryNode* bin, const char* cmp);
int contiene_return(ExpressionNode* expr);
LLVMType tipo_funcion(const char* nombre);
void registrar_funcion(const char* nombre, LLVMType tipo);
LLVMType tipo_param_funcion(const char* nombre, int i);
LLVMType tipo_expr(ExpressionNode* expr);
int es_comparacion(NodeType tipo);
void generar_declaraciones_variables();

// ===== Strings y constantes globales =====

int registrar_string_global(const char* texto);
void recorrer_ast_para_strings(ExpressionNode* expr);
void generar_constantes_globales(ProgramNode* program);

// ===== LLVM funciones externas =====

void declare_extern_functions();
void asegurar_declaracion(const char* var_name);

#endif
