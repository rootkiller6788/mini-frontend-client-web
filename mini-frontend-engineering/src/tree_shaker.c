#include "tree_shaker.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

void shaker_init(TreeShaker *ts) {
    memset(ts, 0, sizeof(*ts));
}

int shaker_add_module(TreeShaker *ts, const char *path, const char *source, size_t len) {
    if (ts->module_count >= MAX_SHAKER_MODULES) return -1;
    ShakerModule *m = &ts->modules[ts->module_count];
    memset(m, 0, sizeof(*m));
    strncpy(m->module_path, path, MAX_PATH_LEN - 1);
    size_t copy_len = len < sizeof(m->source) - 1 ? len : sizeof(m->source) - 1;
    memcpy(m->source, source, copy_len);
    m->source[copy_len] = '\0';
    m->source_len = copy_len;
    m->has_side_effects = true;
    ts->module_count++;
    return 0;
}

void shaker_set_entry(TreeShaker *ts, const char *module_path) {
    for (int i = 0; i < ts->module_count; i++) {
        if (strcmp(ts->modules[i].module_path, module_path) == 0) {
            ts->modules[i].is_bundle_entry = true;
            ts->modules[i].is_reachable = true;
            return;
        }
    }
}

ASTNode* shaker_parse_source(const char *source, size_t len) {
    (void)len;
    ASTNode *root = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!root) return NULL;
    root->kind = AST_NODE_PROGRAM;
    strcpy(root->name, "program");
    root->child_count = 0;

    const char *ptr = source;
    while (*ptr && root->child_count < 32) {
        while (isspace((unsigned char)*ptr)) ptr++;
        if (*ptr == '\0') break;

        const char *kw;
        if (strncmp(ptr, "import ", 7) == 0) {
            ASTNode *n = (ASTNode*)calloc(1, sizeof(ASTNode));
            n->kind = AST_NODE_IMPORT_DECL;
            strncpy(n->name, "import", 127);
            const char *semi = strchr(ptr, ';');
            size_t decl_len = semi ? (size_t)(semi - ptr) : strlen(ptr);
            strncpy(n->value, ptr, decl_len < 255 ? decl_len : 255);
            root->children[root->child_count++] = n;
            ptr = semi ? semi + 1 : ptr + strlen(ptr);
        } else if (strncmp(ptr, "export ", 7) == 0) {
            ASTNode *n = (ASTNode*)calloc(1, sizeof(ASTNode));
            n->kind = AST_NODE_EXPORT_DECL;
            strncpy(n->name, "export", 127);
            const char *semi = strchr(ptr, ';');
            if (!semi) semi = ptr + strlen(ptr);
            size_t decl_len = (size_t)(semi - ptr);
            strncpy(n->value, ptr, decl_len < 255 ? decl_len : 255);
            n->is_used = false;
            root->children[root->child_count++] = n;
            ptr = semi + 1;
        } else if (strncmp(ptr, "const ", 6) == 0 || strncmp(ptr, "let ", 4) == 0 ||
                   strncmp(ptr, "var ", 4) == 0) {
            ASTNode *n = (ASTNode*)calloc(1, sizeof(ASTNode));
            n->kind = AST_NODE_VAR_DECL;
            const char *eq = strchr(ptr, '=');
            const char *semi = strchr(ptr, ';');
            size_t dlen = (semi ? semi : ptr + strlen(ptr)) - ptr;
            strncpy(n->value, ptr, dlen < 255 ? dlen : 255);
            root->children[root->child_count++] = n;
            ptr = semi ? semi + 1 : ptr + strlen(ptr);
            (void)eq;
        } else if (strncmp(ptr, "function ", 9) == 0) {
            ASTNode *n = (ASTNode*)calloc(1, sizeof(ASTNode));
            n->kind = AST_NODE_FUNC_DECL;
            const char *name_start = ptr + 9;
            const char *name_end = strchr(name_start, '(');
            if (name_end) {
                size_t nlen = name_end - name_start;
                strncpy(n->name, name_start, nlen < 127 ? nlen : 127);
            }
            const char *brace = strchr(ptr, '{');
            ptr = brace ? brace + 1 : ptr + strlen(ptr);
            root->children[root->child_count++] = n;
        } else {
            const char *semi = strchr(ptr, ';');
            if (!semi) semi = ptr + strlen(ptr);
            ASTNode *n = (ASTNode*)calloc(1, sizeof(ASTNode));
            n->kind = AST_NODE_EXPR_STMT;
            size_t slen = (size_t)(semi - ptr + 1);
            strncpy(n->value, ptr, slen < 255 ? slen : 255);
            root->children[root->child_count++] = n;
            ptr = semi + 1;
        }
    }
    return root;
}

void shaker_analyze_exports(TreeShaker *ts, ShakerModule *mod) {
    if (!mod->ast) return;
    for (int i = 0; i < mod->ast->child_count; i++) {
        ASTNode *child = mod->ast->children[i];
        if (child->kind != AST_NODE_EXPORT_DECL) continue;
        if (mod->export_count >= MAX_SHAKER_EXPORTS) break;
        ShakerExport *exp = &mod->exports[mod->export_count];
        memset(exp, 0, sizeof(*exp));
        strncpy(exp->source_module, mod->module_path, MAX_PATH_LEN - 1);
        if (strncmp(child->value, "export default", 14) == 0) {
            exp->kind = EXPORT_DEFAULT;
            strcpy(exp->name, "default");
        } else {
            exp->kind = EXPORT_NAMED;
            const char *kw = strchr(child->value + 7, ' ') + 1;
            if (kw) {
                const char *end = kw;
                while (*end && (isalnum((unsigned char)*end) || *end == '_' || *end == '$')) end++;
                size_t nlen = end - kw;
                strncpy(exp->name, kw, nlen < 127 ? nlen : 127);
            } else {
                snprintf(exp->name, 127, "export_%d", mod->export_count);
            }
        }
        exp->decl_node = child;
        exp->is_used = false;
        exp->is_reachable = mod->is_reachable;
        mod->export_count++;
    }
    (void)ts;
}

void shaker_mark_reachable(TreeShaker *ts) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < ts->module_count; i++) {
            ShakerModule *m = &ts->modules[i];
            if (!m->is_reachable) continue;
            for (int d = 0; d < m->dependency_count; d++) {
                for (int j = 0; j < ts->module_count; j++) {
                    if (strcmp(ts->modules[j].module_path, m->dependencies[d]) == 0) {
                        if (!ts->modules[j].is_reachable) {
                            ts->modules[j].is_reachable = true;
                            changed = true;
                        }
                    }
                }
            }
        }
    }
}

void shaker_mark_used_exports(TreeShaker *ts) {
    for (int i = 0; i < ts->module_count; i++) {
        ShakerModule *m = &ts->modules[i];
        if (!m->is_reachable) continue;
        if (m->is_bundle_entry) {
            for (int e = 0; e < m->export_count; e++)
                m->exports[e].is_used = true;
        }
        for (int imp = 0; imp < m->import_count; imp++) {
            ShakerImport *im = &m->imports[imp];
            for (int j = 0; j < ts->module_count; j++) {
                if (strcmp(ts->modules[j].module_path, im->source_module) != 0) continue;
                for (int e = 0; e < ts->modules[j].export_count; e++) {
                    for (int b = 0; b < im->bound_count; b++) {
                        if (strcmp(ts->modules[j].exports[e].name, im->bound_names[b]) == 0) {
                            ts->modules[j].exports[e].is_used = true;
                            ts->modules[j].exports[e].usage_count++;
                        }
                    }
                }
            }
        }
    }
}

bool shaker_is_side_effect_free(TreeShaker *ts, const char *module_path) {
    for (int i = 0; i < ts->side_effect_count; i++) {
        size_t plen = strlen(ts->side_effects[i].file_pattern);
        size_t mlen = strlen(module_path);
        if (mlen >= plen) {
            const char *suffix = module_path + mlen - plen;
            if (strcmp(suffix, ts->side_effects[i].file_pattern) == 0)
                return !ts->side_effects[i].has_side_effects;
        }
    }
    return false;
}

void shaker_eliminate_dead_code(TreeShaker *ts) {
    for (int i = 0; i < ts->module_count; i++) {
        ShakerModule *m = &ts->modules[i];
        if (!m->is_reachable) {
            m->stripped_len = 0;
            m->stripped_source[0] = '\0';
            ts->total_stripped_size += (int)m->source_len;
            continue;
        }

        char *out = m->stripped_source;
        size_t remaining = sizeof(m->stripped_source) - 1;

        for (int e = 0; e < m->export_count; e++) {
            if (m->exports[e].is_used && m->exports[e].decl_node) {
                size_t vlen = strlen(m->exports[e].decl_node->value);
                if (vlen < remaining) {
                    memcpy(out + strlen(out), m->exports[e].decl_node->value, vlen);
                    size_t cur = strlen(out);
                    out[cur] = '\n';
                    out[cur + 1] = '\0';
                }
            }
        }

        if (strlen(out) == 0) {
            if (shaker_is_side_effect_free(ts, m->module_path)) {
                m->stripped_len = 0;
                m->stripped_source[0] = '\0';
            } else {
                size_t slen = m->source_len;
                memcpy(out, m->source, slen);
                m->stripped_len = slen;
            }
        }
        m->stripped_len = strlen(m->stripped_source);
        ts->total_stripped_size += (int)m->stripped_len;
        ts->total_original_size += (int)m->source_len;
    }

    if (ts->total_original_size > 0)
        ts->reduction_ratio = 1.0 - (double)ts->total_stripped_size / ts->total_original_size;
}

void shaker_scope_hoisting(TreeShaker *ts) {
    char hoisted[262144];
    size_t hlen = 0;
    for (int i = 0; i < ts->module_count; i++) {
        ShakerModule *m = &ts->modules[i];
        if (!m->is_reachable || m->stripped_len == 0) continue;
        size_t mlen = m->stripped_len;
        if (hlen + mlen + 64 >= sizeof(hoisted)) break;
        snprintf(hoisted + hlen, sizeof(hoisted) - hlen,
            "/* module:%s */\n%s\n", m->module_path, m->stripped_source);
        hlen = strlen(hoisted);
    }
    if (ts->module_count > 0) {
        ShakerModule *first = &ts->modules[0];
        memset(first->stripped_source, 0, sizeof(first->stripped_source));
        strncpy(first->stripped_source, hoisted, sizeof(first->stripped_source) - 1);
        first->stripped_len = strlen(first->stripped_source);
    }
    (void)ts;
}

int shaker_parse_package_json(TreeShaker *ts, const char *pkg_path) {
    FILE *f = fopen(pkg_path, "rb");
    if (!f) return -1;
    char buf[16384];
    size_t r = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[r] = '\0';

    const char *side = strstr(buf, "\"sideEffects\"");
    if (!side) return 0;

    const char *val = strchr(side, ':');
    if (!val) return 0;

    if (strstr(val, "false")) {
        if (ts->side_effect_count < MAX_SIDE_EFFECT_FILES) {
            SideEffectEntry *se = &ts->side_effects[ts->side_effect_count];
            strcpy(se->file_pattern, "*");
            se->has_side_effects = false;
            ts->side_effect_count++;
        }
    }

    return 0;
}

void shaker_free_ast(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++)
        shaker_free_ast(node->children[i]);
    free(node);
}

void shaker_print_stats(TreeShaker *ts) {
    printf("Tree Shaker Stats:\n");
    printf("  Modules: %d\n", ts->module_count);
    printf("  Original size: %d bytes\n", ts->total_original_size);
    printf("  Stripped size: %d bytes\n", ts->total_stripped_size);
    printf("  Reduction: %.1f%%\n", ts->reduction_ratio * 100.0);
    printf("  Reachable exports: %d\n", ts->reachable_count);
    for (int i = 0; i < ts->module_count; i++) {
        ShakerModule *m = &ts->modules[i];
        printf("  %s: %zu -> %zu bytes, used=%d\n",
            m->module_path, m->source_len, m->stripped_len, m->is_reachable);
    }
}
