#ifndef BUNDLER_CORE_H
#define BUNDLER_CORE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_MODULES         1024
#define MAX_DEPENDENCIES    256
#define MAX_PATH_LEN        512
#define MAX_EXPORT_COUNT    128
#define MAX_IMPORT_COUNT    128
#define MAX_CHUNKS          64
#define MAX_CHUNK_MODULES   256

typedef enum {
    MODULE_ESM,
    MODULE_CJS,
    MODULE_UNKNOWN
} ModuleType;

typedef enum {
    CHUNK_ENTRY,
    CHUNK_VENDOR,
    CHUNK_APP,
    CHUNK_DYNAMIC
} ChunkType;

typedef struct {
    char id[MAX_PATH_LEN];
    char source[65536];
    size_t source_len;
    ModuleType type;
    int dependency_count;
    char dependencies[MAX_DEPENDENCIES][MAX_PATH_LEN];
    int export_count;
    char exports[MAX_EXPORT_COUNT][128];
    int import_count;
    char import_specifiers[MAX_IMPORT_COUNT][128];
    char import_sources[MAX_IMPORT_COUNT][MAX_PATH_LEN];
    bool is_entry;
    bool is_dynamic;
    bool is_node_module;
    int chunk_id;
} Module;

typedef struct {
    char id[MAX_PATH_LEN];
    Module modules[MAX_CHUNK_MODULES];
    int module_count;
    ChunkType type;
    char output_path[MAX_PATH_LEN];
    char bundled_source[131072];
    size_t bundled_len;
    bool is_async;
    int priority;
} Chunk;

typedef struct {
    char root_dir[MAX_PATH_LEN];
    char entry_points[MAX_DEPENDENCIES][MAX_PATH_LEN];
    int entry_count;
    char node_modules_path[MAX_PATH_LEN];
    char output_dir[MAX_PATH_LEN];
    char public_path[256];
    Module modules[MAX_MODULES];
    int module_count;
    Chunk chunks[MAX_CHUNKS];
    int chunk_count;
    int vendor_chunk_id;
    int app_chunk_id;
    bool minify;
    bool sourcemap;
    bool splitting;
    bool tree_shake;
} BundlerConfig;

typedef struct {
    char alias[128];
    char target[MAX_PATH_LEN];
} PathAlias;

typedef struct {
    PathAlias aliases[64];
    int alias_count;
    char resolve_extensions[8][8];
    int ext_count;
    bool prefer_main;
    bool prefer_module;
} ResolveConfig;

typedef struct {
    BundlerConfig config;
    ResolveConfig resolve;
    char resolve_cache[2048][2][MAX_PATH_LEN];
    int cache_count;
} Bundler;

void bundler_init(Bundler *b, const char *root);
int  bundler_add_entry(Bundler *b, const char *entry_path);
int  bundler_resolve_module(Bundler *b, const char *base_path, const char *specifier, char *out_path, size_t out_len);
int  bundler_parse_module(Bundler *b, const char *module_path, Module *mod);
int  bundler_build_dependency_graph(Bundler *b);
void bundler_split_chunks(Bundler *b);
int  bundler_generate_bundle(Bundler *b);
void bundler_write_output(Bundler *b, const char *output_dir);
void bundler_print_stats(Bundler *b);

#endif
