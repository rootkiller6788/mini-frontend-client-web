#include "bundler_core.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int mod_index(Bundler *b, const char *id) {
    for (int i = 0; i < b->config.module_count; i++) {
        if (strcmp(b->config.modules[i].id, id) == 0) return i;
    }
    return -1;
}

static bool has_ext(const char *path) {
    const char *dot = strrchr(path, '.');
    return dot && dot != path && *(dot - 1) != '/' && *(dot - 1) != '\\';
}

void bundler_init(Bundler *b, const char *root) {
    memset(b, 0, sizeof(*b));
    strncpy(b->config.root_dir, root, MAX_PATH_LEN - 1);
    strcpy(b->config.output_dir, "dist");
    strcpy(b->config.public_path, "/");
    strcpy(b->resolve.resolve_extensions[0], ".js");
    strcpy(b->resolve.resolve_extensions[1], ".ts");
    strcpy(b->resolve.resolve_extensions[2], ".json");
    b->resolve.ext_count = 3;
    b->resolve.prefer_main = true;
    b->resolve.prefer_module = false;
    b->config.sourcemap = true;
    b->config.splitting = true;
    b->config.tree_shake = true;
}

int bundler_add_entry(Bundler *b, const char *entry_path) {
    if (b->config.entry_count >= MAX_DEPENDENCIES) return -1;
    strncpy(b->config.entry_points[b->config.entry_count], entry_path, MAX_PATH_LEN - 1);
    b->config.entry_count++;
    return 0;
}

static int resolve_cache_put(Bundler *b, const char *base, const char *spec, const char *result) {
    if (b->cache_count >= 2048) return -1;
    strncpy(b->resolve_cache[b->cache_count][0], base, MAX_PATH_LEN - 1);
    strncpy(b->resolve_cache[b->cache_count][1], spec, MAX_PATH_LEN - 1);
    b->cache_count++;
    (void)result;
    return 0;
}

static const char* resolve_cache_get(Bundler *b, const char *base, const char *spec) {
    for (int i = 0; i < b->cache_count; i++) {
        if (strcmp(b->resolve_cache[i][0], base) == 0 &&
            strcmp(b->resolve_cache[i][1], spec) == 0)
            return b->resolve_cache[i][1]; /* placeholder */
    }
    return NULL;
    (void)spec;
}

int bundler_resolve_module(Bundler *b, const char *base_path, const char *specifier, char *out_path, size_t out_len) {
    const char *cached = resolve_cache_get(b, base_path, specifier);
    if (cached) { strncpy(out_path, cached, out_len); return 0; }
    (void)cached;

    char base_dir[MAX_PATH_LEN];
    strncpy(base_dir, base_path, MAX_PATH_LEN - 1);
    char *last_sep = strrchr(base_dir, '/');
    if (!last_sep) last_sep = strrchr(base_dir, '\\');
    if (last_sep) *last_sep = '\0';

    if (specifier[0] == '.' || specifier[0] == '/') {
        char candidate[MAX_PATH_LEN];
        int n = snprintf(candidate, MAX_PATH_LEN, "%s/%s", base_dir, specifier);
        if (n < 0 || n >= MAX_PATH_LEN) return -1;

        for (int i = 0; i < b->resolve.ext_count; i++) {
            char with_ext[MAX_PATH_LEN];
            snprintf(with_ext, MAX_PATH_LEN, "%s%s", candidate, b->resolve.resolve_extensions[i]);
            FILE *f = fopen(with_ext, "r");
            if (f) { fclose(f); strncpy(out_path, with_ext, out_len); return 0; }
        }
        char index_candidate[MAX_PATH_LEN];
        snprintf(index_candidate, MAX_PATH_LEN, "%s/index.js", candidate);
        FILE *f = fopen(index_candidate, "r");
        if (f) { fclose(f); strncpy(out_path, index_candidate, out_len); return 0; }
    }

    if (specifier[0] != '.' && specifier[0] != '/') {
        char node_path[MAX_PATH_LEN];
        snprintf(node_path, MAX_PATH_LEN, "%s/node_modules/%s", b->config.root_dir, specifier);
        char pkg_json[MAX_PATH_LEN];
        snprintf(pkg_json, MAX_PATH_LEN, "%s/package.json", node_path);
        FILE *pkg = fopen(pkg_json, "r");
        if (pkg) {
            fclose(pkg);
            char bundle_path[MAX_PATH_LEN];
            snprintf(bundle_path, MAX_PATH_LEN, "%s/index.js", node_path);
            FILE *bf = fopen(bundle_path, "r");
            if (bf) { fclose(bf); strncpy(out_path, bundle_path, out_len); return 0; }
        }
    }
    return -1;
}

static int parse_imports(const char *source, Module *mod) {
    const char *ptr = source;
    while ((ptr = strstr(ptr, "import")) && (mod->import_count < MAX_IMPORT_COUNT)) {
        const char *from = strstr(ptr, "from");
        if (!from) break;
        const char *q1 = strchr(from, '\'');
        const char *q2 = strchr(from, '"');
        const char *q = q1 && q2 ? (q1 < q2 ? q1 : q2) : (q1 ? q1 : q2);
        if (!q) break;
        const char *qe = strchr(q + 1, *q);
        if (!qe) break;
        size_t slen = qe - q - 1;
        if (slen >= MAX_PATH_LEN) slen = MAX_PATH_LEN - 1;
        strncpy(mod->import_sources[mod->import_count], q + 1, slen);
        mod->import_sources[mod->import_count][slen] = '\0';
        mod->import_count++;
        ptr = qe + 1;
    }
    return 0;
}

static int parse_exports(const char *source, Module *mod) {
    const char *ptr = source;
    while ((ptr = strstr(ptr, "export")) && (mod->export_count < MAX_EXPORT_COUNT)) {
        if (strncmp(ptr, "export default", 14) == 0) {
            strcpy(mod->exports[mod->export_count], "default");
            mod->export_count++;
        } else if (strncmp(ptr, "export const", 12) == 0 || strncmp(ptr, "export let", 10) == 0 ||
                   strncmp(ptr, "export var", 10) == 0 || strncmp(ptr, "export function", 15) == 0 ||
                   strncmp(ptr, "export class", 12) == 0) {
            const char *kw = strchr(ptr + 7, ' ') + 1;
            if (kw) {
                const char *end = kw;
                while (*end && (isalnum((unsigned char)*end) || *end == '_' || *end == '$')) end++;
                size_t nlen = end - kw;
                if (nlen > 0 && nlen < 128) {
                    strncpy(mod->exports[mod->export_count], kw, nlen);
                    mod->exports[mod->export_count][nlen] = '\0';
                    mod->export_count++;
                }
            }
        }
        ptr += 6;
    }
    return 0;
}

int bundler_parse_module(Bundler *b, const char *module_path, Module *mod) {
    memset(mod, 0, sizeof(*mod));
    strncpy(mod->id, module_path, MAX_PATH_LEN - 1);

    FILE *f = fopen(module_path, "rb");
    if (!f) return -1;
    size_t r = fread(mod->source, 1, sizeof(mod->source) - 1, f);
    fclose(f);
    mod->source[r] = '\0';
    mod->source_len = r;

    if (strstr(mod->source, "require(") || strstr(mod->source, "module.exports"))
        mod->type = MODULE_CJS;
    else if (strstr(mod->source, "import ") || strstr(mod->source, "export "))
        mod->type = MODULE_ESM;
    else
        mod->type = MODULE_UNKNOWN;

    mod->is_node_module = strstr(module_path, "node_modules") != NULL;

    const char *src = mod->source;
    const char *req;
    while ((req = strstr(src, "require(")) && (mod->dependency_count < MAX_DEPENDENCIES)) {
        const char *q = strchr(req, '\'');
        if (!q) q = strchr(req, '"');
        if (!q) break;
        const char *qe = strchr(q + 1, *q);
        if (!qe) break;
        size_t slen = qe - q - 1;
        strncpy(mod->dependencies[mod->dependency_count], q + 1, slen);
        mod->dependencies[mod->dependency_count][slen] = '\0';
        mod->dependency_count++;
        src = qe + 1;
    }

    parse_imports(mod->source, mod);
    for (int i = 0; i < mod->import_count; i++) {
        if (mod->dependency_count < MAX_DEPENDENCIES) {
            strcpy(mod->dependencies[mod->dependency_count], mod->import_sources[i]);
            mod->dependency_count++;
        }
    }
    parse_exports(mod->source, mod);
    return 0;
}

int bundler_build_dependency_graph(Bundler *b) {
    for (int i = 0; i < b->config.entry_count; i++) {
        if (b->config.module_count >= MAX_MODULES) break;
        Module *m = &b->config.modules[b->config.module_count];
        bundler_parse_module(b, b->config.entry_points[i], m);
        m->is_entry = true;
        b->config.module_count++;

        for (int d = 0; d < m->dependency_count; d++) {
            if (b->config.module_count >= MAX_MODULES) break;
            char resolved[MAX_PATH_LEN];
            if (bundler_resolve_module(b, m->id, m->dependencies[d], resolved, sizeof(resolved)) == 0) {
                int idx = mod_index(b, resolved);
                if (idx < 0) {
                    Module *dep = &b->config.modules[b->config.module_count];
                    bundler_parse_module(b, resolved, dep);
                    b->config.module_count++;
                }
            }
        }
    }
    return 0;
}

void bundler_split_chunks(Bundler *b) {
    if (!b->config.splitting) return;
    b->config.vendor_chunk_id = -1;
    b->config.app_chunk_id = -1;

    b->config.vendor_chunk_id = b->config.chunk_count;
    Chunk *vc = &b->config.chunks[b->config.chunk_count];
    vc->type = CHUNK_VENDOR;
    strcpy(vc->id, "vendor");
    strcpy(vc->output_path, "vendor.bundle.js");
    vc->is_async = false;
    vc->priority = 10;
    vc->module_count = 0;
    b->config.chunk_count++;

    b->config.app_chunk_id = b->config.chunk_count;
    Chunk *ac = &b->config.chunks[b->config.chunk_count];
    ac->type = CHUNK_APP;
    strcpy(ac->id, "app");
    strcpy(ac->output_path, "app.bundle.js");
    ac->is_async = false;
    ac->priority = 5;
    ac->module_count = 0;
    b->config.chunk_count++;

    for (int i = 0; i < b->config.module_count; i++) {
        Module *m = &b->config.modules[i];
        if (m->is_node_module) {
            if (vc->module_count < MAX_CHUNK_MODULES) {
                vc->modules[vc->module_count] = *m;
                vc->module_count++;
                m->chunk_id = b->config.vendor_chunk_id;
            }
        } else {
            if (ac->module_count < MAX_CHUNK_MODULES) {
                ac->modules[ac->module_count] = *m;
                ac->module_count++;
                m->chunk_id = b->config.app_chunk_id;
            }
        }
    }
}

static void append_iife_wrapper(char *buf, size_t *len, const char *module_code, int module_id) {
    (void)module_id;
    size_t code_len = strlen(module_code);
    if (*len + code_len + 256 > 131072) return;
    snprintf(buf + *len, 131072 - *len,
        "(function(modules,entryMap,chunkMap){(function(__modules__){\n"
        "var installed={};\n"
        "function __require__(id){\n"
        "if(installed[id])return installed[id].exports;\n"
        "var m=installed[id]={i:id,l:false,exports:{}};\n"
        "__modules__[id].call(m.exports,m,m.exports,__require__);\n"
        "m.l=true;return m.exports;\n"
        "}\n"
        "%s\n"
        "})([\n"
        "function(module,exports,require){\n%s}\n"
        "]);\n})(modules,entryMap,chunkMap);\n",
        module_code);
    *len = strlen(buf);
}

int bundler_generate_bundle(Bundler *b) {
    for (int c = 0; c < b->config.chunk_count; c++) {
        Chunk *ch = &b->config.chunks[c];
        ch->bundled_len = 0;
        memset(ch->bundled_source, 0, sizeof(ch->bundled_source));

        for (int m = 0; m < ch->module_count; m++) {
            Module *mod = &ch->modules[m];
            size_t cur = ch->bundled_len;
            append_iife_wrapper(ch->bundled_source, &cur, mod->source, m);
            ch->bundled_len = cur;
        }
    }
    return 0;
}

void bundler_write_output(Bundler *b, const char *output_dir) {
    for (int i = 0; i < b->config.chunk_count; i++) {
        Chunk *ch = &b->config.chunks[i];
        char full_path[MAX_PATH_LEN];
        snprintf(full_path, MAX_PATH_LEN, "%s/%s", output_dir, ch->output_path);
        FILE *f = fopen(full_path, "wb");
        if (!f) continue;
        fwrite(ch->bundled_source, 1, ch->bundled_len, f);
        fclose(f);
    }
}

void bundler_print_stats(Bundler *b) {
    printf("Bundle Stats:\n");
    printf("  Entries: %d\n", b->config.entry_count);
    printf("  Modules: %d\n", b->config.module_count);
    printf("  Chunks:  %d\n", b->config.chunk_count);
    for (int i = 0; i < b->config.chunk_count; i++) {
        Chunk *ch = &b->config.chunks[i];
        printf("  Chunk[%d] %s (%s): %d modules, %zu bytes\n",
            i, ch->id, ch->output_path, ch->module_count, ch->bundled_len);
    }
    size_t total = 0;
    for (int i = 0; i < b->config.chunk_count; i++)
        total += b->config.chunks[i].bundled_len;
    printf("  Total: %zu bytes\n", total);
}
