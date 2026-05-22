#ifndef TREE_SHAKER_H
#define TREE_SHAKER_H

#include <stddef.h>
#include <stdbool.h>

#define MAX_SHAKER_MODULES      1024
#define MAX_SHAKER_EXPORTS      256
#define MAX_SHAKER_IMPORTS      256
#define MAX_SIDE_EFFECT_FILES   128
#define MAX_SCOPE_VARS          512
#define MAX_REACHABLE_EXPORTS   2048

typedef enum {
    EXPORT_NAMED,
    EXPORT_DEFAULT,
    EXPORT_ALL,
    EXPORT_NAMESPACE
} ExportKind;

typedef enum {
    IMPORT_NAMED,
    IMPORT_DEFAULT,
    IMPORT_NAMESPACE,
    IMPORT_DYNAMIC
} ImportKind;

typedef enum {
    AST_NODE_PROGRAM,
    AST_NODE_IMPORT_DECL,
    AST_NODE_EXPORT_DECL,
    AST_NODE_VAR_DECL,
    AST_NODE_FUNC_DECL,
    AST_NODE_CLASS_DECL,
    AST_NODE_EXPR_STMT,
    AST_NODE_CALL_EXPR,
    AST_NODE_MEMBER_EXPR,
    AST_NODE_IDENTIFIER,
    AST_NODE_LITERAL,
    AST_NODE_BLOCK,
    AST_NODE_RETURN,
    AST_NODE_IF_STMT,
    AST_NODE_FOR_STMT
} ASTNodeKind;

typedef struct ASTNode {
    ASTNodeKind kind;
    char name[128];
    char value[256];
    int child_count;
    struct ASTNode *children[32];
    bool is_used;
    bool has_side_effects;
    int line;
    int column;
} ASTNode;

typedef struct {
    char name[128];
    char source_module[MAX_PATH_LEN];
    ExportKind kind;
    bool is_reachable;
    bool is_used;
    ASTNode *decl_node;
    int usage_count;
} ShakerExport;

typedef struct {
    char name[128];
    char source_module[MAX_PATH_LEN];
    ImportKind kind;
    bool is_reachable;
    bool is_used;
    char bound_names[32][128];
    int bound_count;
} ShakerImport;

typedef struct {
    char module_path[MAX_PATH_LEN];
    char source[65536];
    size_t source_len;
    ASTNode *ast;
    ShakerExport exports[MAX_SHAKER_EXPORTS];
    int export_count;
    ShakerImport imports[MAX_SHAKER_IMPORTS];
    int import_count;
    bool has_side_effects;
    bool is_reachable;
    bool is_bundle_entry;
    int dependency_count;
    char dependencies[64][MAX_PATH_LEN];
    char stripped_source[65536];
    size_t stripped_len;
} ShakerModule;

typedef struct {
    char file_pattern[256];
    bool has_side_effects;
} SideEffectEntry;

typedef struct {
    ShakerModule modules[MAX_SHAKER_MODULES];
    int module_count;
    SideEffectEntry side_effects[MAX_SIDE_EFFECT_FILES];
    int side_effect_count;
    char reachable_exports[MAX_REACHABLE_EXPORTS][256];
    int reachable_count;
    int total_original_size;
    int total_stripped_size;
    double reduction_ratio;
} TreeShaker;

void shaker_init(TreeShaker *ts);
int  shaker_add_module(TreeShaker *ts, const char *path, const char *source, size_t len);
void shaker_set_entry(TreeShaker *ts, const char *module_path);
int  shaker_parse_package_json(TreeShaker *ts, const char *pkg_path);
ASTNode* shaker_parse_source(const char *source, size_t len);
void shaker_analyze_exports(TreeShaker *ts, ShakerModule *mod);
void shaker_mark_reachable(TreeShaker *ts);
void shaker_mark_used_exports(TreeShaker *ts);
bool shaker_is_side_effect_free(TreeShaker *ts, const char *module_path);
void shaker_eliminate_dead_code(TreeShaker *ts);
void shaker_scope_hoisting(TreeShaker *ts);
void shaker_print_stats(TreeShaker *ts);
void shaker_free_ast(ASTNode *node);

#endif
