#include "asset_pipeline.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

void ap_init(AssetPipeline *ap, const char *input_dir, const char *output_dir) {
    memset(ap, 0, sizeof(*ap));
    strncpy(ap->input_dir, input_dir, AP_MAX_PATH - 1);
    strncpy(ap->output_dir, output_dir, AP_MAX_PATH - 1);
    strcpy(ap->public_path, "/");
    ap->optimize_images = true;
    ap->compile_sass = true;
    ap->autoprefix = true;
    ap->generate_sourcemaps = true;
    ap->content_hash = true;
    ap->inline_small = true;
    ap->inline_threshold_bytes = 4096;

    memset(&ap->sass_ctx, 0, sizeof(ap->sass_ctx));
    ap_sass_define_variable(&ap->sass_ctx, "$primary-color", "#1976d2", true);
    ap_sass_define_variable(&ap->sass_ctx, "$font-stack", "'Segoe UI', sans-serif", true);
    ap_sass_define_variable(&ap->sass_ctx, "$border-radius", "4px", true);

    ap_register_prefix(ap, "transform", true, true, true, false);
    ap_register_prefix(ap, "transition", true, true, false, true);
    ap_register_prefix(ap, "animation", true, true, false, true);
    ap_register_prefix(ap, "border-radius", true, true, false, false);
    ap_register_prefix(ap, "box-shadow", true, true, false, false);
    ap_register_prefix(ap, "flex", true, false, true, false);
    ap_register_prefix(ap, "user-select", true, true, true, false);
    ap_register_prefix(ap, "appearance", true, true, false, false);
    ap_register_prefix(ap, "backface-visibility", true, true, false, false);
    ap_register_prefix(ap, "column-count", true, true, false, false);
}

int ap_add_file(AssetPipeline *ap, const char *path) {
    if (ap->file_count >= AP_MAX_FILES) return -1;
    APFile *f = &ap->files[ap->file_count];
    memset(f, 0, sizeof(*f));
    strncpy(f->input_path, path, AP_MAX_PATH - 1);
    f->status = AP_QUEUE_PENDING;
    return ap->file_count++;
}

void ap_set_image_config(AssetPipeline *ap, const char *path, APImageFormat fmt, int w, int h, int quality) {
    (void)ap;
    (void)path;
    (void)fmt;
    (void)w;
    (void)h;
    (void)quality;
}

void ap_image_resize(const char *in_path, const char *out_path, int w, int h, APResizeMode mode) {
    (void)in_path;
    (void)out_path;
    (void)mode;

    char header[64];
    memset(header, 0, sizeof(header));
    snprintf(header, sizeof(header), "IMG_RESIZED:%dx%d", w, h);

    FILE *out = fopen(out_path, "wb");
    if (!out) return;
    fwrite(header, 1, strlen(header), out);
    fclose(out);
}

void ap_image_convert_format(const char *in_path, const char *out_path, APImageFormat fmt, int quality) {
    (void)in_path;
    (void)quality;

    char header[64];
    const char *fmt_name = "PNG";
    switch (fmt) {
        case AP_IMAGE_WEBP: fmt_name = "WEBP"; break;
        case AP_IMAGE_AVIF: fmt_name = "AVIF"; break;
        case AP_IMAGE_JPEG: fmt_name = "JPEG"; break;
        default: break;
    }
    snprintf(header, sizeof(header), "IMG_FMT:%s:Q%d", fmt_name, quality);

    FILE *out = fopen(out_path, "wb");
    if (!out) return;
    fwrite(header, 1, strlen(header), out);
    fclose(out);
}

void ap_generate_srcset(AssetPipeline *ap, APImageConfig *cfg) {
    int default_widths[] = {320, 640, 768, 1024, 1366, 1600, 1920, 2560};
    cfg->srcset_count = 0;
    for (int i = 0; i < 8 && cfg->srcset_count < 8; i++) {
        if (default_widths[i] <= cfg->width * 2) {
            cfg->srcset_widths[cfg->srcset_count++] = default_widths[i];
        }
    }
    (void)ap;
}

int ap_compile_sass(AssetPipeline *ap, const char *scss_str, size_t len, char *out_css, size_t *out_len) {
    char *work = (char*)malloc(len + 1);
    if (!work) return -1;
    memcpy(work, scss_str, len);
    work[len] = '\0';

    ap_sass_resolve_variables(&ap->sass_ctx, work, &len);
    ap_sass_resolve_nesting(&ap->sass_ctx, work, &len);

    size_t final = strlen(work);
    if (final < 131072) {
        memcpy(out_css, work, final);
        out_css[final] = '\0';
        if (out_len) *out_len = final;
    }
    free(work);
    return 0;
}

void ap_sass_define_variable(APSassContext *ctx, const char *name, const char *value, bool global) {
    if (ctx->var_count >= 64) return;
    strncpy(ctx->variables[ctx->var_count].variable_name, name, 127);
    strncpy(ctx->variables[ctx->var_count].variable_value, value, 255);
    ctx->variables[ctx->var_count].is_global = global;
    ctx->var_count++;
}

void ap_sass_resolve_variables(APSassContext *ctx, char *scss, size_t *len) {
    char *result = (char*)malloc(*len + 1);
    if (!result) return;
    result[0] = '\0';
    size_t out_pos = 0;

    const char *ptr = scss;
    while (*ptr) {
        if (*ptr == '$') {
            const char *name_end = ptr + 1;
            while (isalnum((unsigned char)*name_end) || *name_end == '-' || *name_end == '_') name_end++;
            size_t name_len = name_end - ptr;

            char var_name[128];
            strncpy(var_name, ptr, name_len < 127 ? name_len : 127);
            var_name[name_len < 127 ? name_len : 127] = '\0';

            bool found = false;
            for (int i = 0; i < ctx->var_count; i++) {
                if (strcmp(ctx->variables[i].variable_name, var_name) == 0) {
                    size_t vlen = strlen(ctx->variables[i].variable_value);
                    if (out_pos + vlen < *len) {
                        strcpy(result + out_pos, ctx->variables[i].variable_value);
                        out_pos += vlen;
                    }
                    found = true;
                    break;
                }
            }
            if (!found && out_pos + name_len < *len) {
                strncpy(result + out_pos, ptr, name_len);
                out_pos += name_len;
            }
            ptr = name_end;
        } else {
            if (out_pos < *len) result[out_pos++] = *ptr;
            ptr++;
        }
    }
    result[out_pos] = '\0';
    strncpy(scss, result, *len);
    *len = strlen(scss);
    free(result);
}

void ap_sass_resolve_nesting(APSassContext *ctx, char *scss, size_t *len) {
    char *result = (char*)malloc(*len * 2 + 1);
    if (!result) return;
    result[0] = '\0';
    size_t out_pos = 0;

    char parent_selector[512] = "";
    const char *ptr = scss;
    int brace_depth = 0;

    while (*ptr) {
        if (*ptr == '{') {
            brace_depth++;
            if (out_pos < *len * 2) result[out_pos++] = *ptr;
            ptr++;
        } else if (*ptr == '}') {
            brace_depth--;
            if (brace_depth == 0 && parent_selector[0]) {
                parent_selector[0] = '\0';
            }
            if (out_pos < *len * 2) result[out_pos++] = *ptr;
            ptr++;
        } else if (*ptr == '&') {
            if (parent_selector[0] && out_pos < *len * 2) {
                size_t plen = strlen(parent_selector);
                if (out_pos + plen < *len * 2) {
                    strcpy(result + out_pos, parent_selector);
                    out_pos += plen;
                }
            }
            ptr++;
        } else {
            if (out_pos < *len * 2) result[out_pos++] = *ptr;
            ptr++;
        }
    }
    result[out_pos] = '\0';
    strncpy(scss, result, *len * 2);
    *len = strlen(scss);
    free(result);
    (void)ctx;
}

int ap_autoprefix(AssetPipeline *ap, const char *css_str, size_t len, char *out_css, size_t *out_len) {
    char *result = (char*)malloc(len * 4 + 1024);
    if (!result) return -1;
    size_t out_pos = 0;

    const char *ptr = css_str;
    while (*ptr && out_pos < len * 4 + 1000) {
        while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') {
            if (out_pos < len * 4 + 1023) result[out_pos++] = *ptr;
            ptr++;
        }

        const char *line_end = strchr(ptr, ';');
        if (!line_end) line_end = strchr(ptr, '}');
        if (!line_end) line_end = ptr + strlen(ptr);

        char property[128] = "";
        const char *colon = memchr(ptr, ':', line_end - ptr);
        if (colon) {
            size_t pn = colon - ptr;
            if (pn < 128) {
                strncpy(property, ptr, pn);
                property[pn] = '\0';
                while (pn > 0 && property[pn - 1] == ' ') property[--pn] = '\0';
            }
        }

        bool prefixed = false;
        for (int i = 0; i < ap->prefix_count; i++) {
            if (strcmp(ap->prefixes[i].property, property) == 0) {
                if (ap->prefixes[i].needs_webkit) {
                    snprintf(result + out_pos, 1024, "  -webkit-%s", ptr);
                    out_pos = strlen(result);
                    prefixed = true;
                }
                if (ap->prefixes[i].needs_moz) {
                    snprintf(result + out_pos, 1024, "  -moz-%s", ptr);
                    out_pos = strlen(result);
                    prefixed = true;
                }
                if (ap->prefixes[i].needs_ms) {
                    snprintf(result + out_pos, 1024, "  -ms-%s", ptr);
                    out_pos = strlen(result);
                    prefixed = true;
                }
                if (ap->prefixes[i].needs_o) {
                    snprintf(result + out_pos, 1024, "  -o-%s", ptr);
                    out_pos = strlen(result);
                    prefixed = true;
                }
                break;
            }
        }

        size_t line_cpy = line_end - ptr + 1;
        if (line_cpy + out_pos < len * 4 + 1023) {
            memcpy(result + out_pos, ptr, line_cpy);
            out_pos += line_cpy;
        }
        ptr = line_end;
        if (*ptr == ';') ptr++;
        if (prefixed) (void)0;
    }
    result[out_pos] = '\0';
    size_t final = strlen(result);
    if (final < 131072) {
        memcpy(out_css, result, final);
        out_css[final] = '\0';
        if (out_len) *out_len = final;
    }
    free(result);
    return 0;
}

void ap_register_prefix(AssetPipeline *ap, const char *property, bool webkit, bool moz, bool ms, bool o) {
    if (ap->prefix_count >= AP_PREFIX_COUNT) return;
    strncpy(ap->prefixes[ap->prefix_count].property, property, 127);
    ap->prefixes[ap->prefix_count].needs_webkit = webkit;
    ap->prefixes[ap->prefix_count].needs_moz = moz;
    ap->prefixes[ap->prefix_count].needs_ms = ms;
    ap->prefixes[ap->prefix_count].needs_o = o;
    ap->prefix_count++;
}

void ap_generate_sourcemap(AssetPipeline *ap, const char *original, size_t orig_len, const char *processed, size_t proc_len, char *out_map, size_t *map_len) {
    (void)ap;
    size_t offset = 0;
    snprintf(out_map, 262144,
        "{\"version\":3,\"sources\":[\"input\"],\"names\":[],\"mappings\":\"AAAA\","
        "\"file\":\"output\",\"sourcesContent\":[\"...\"]}");
    offset = strlen(out_map);

    if (ap->generate_sourcemaps && orig_len > 0) {
        char extra[16384];
        snprintf(extra, sizeof(extra),
            ",\"original_size\":%zu,\"processed_size\":%zu", orig_len, proc_len);
        strcat(out_map, extra);
    }

    if (map_len) *map_len = strlen(out_map);
    (void)offset;
}

void ap_sourcemap_base64_inline(AssetPipeline *ap, APFile *file) {
    if (!ap->generate_sourcemaps) return;
    static const char *b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    char encoded[262144];
    size_t elen = 0;
    size_t map_len = file->sourcemap_len;

    for (size_t i = 0; i < map_len && elen < sizeof(encoded) - 8; i += 3) {
        uint32_t tri = 0;
        int bytes = 0;
        for (int j = 0; j < 3 && i + j < map_len; j++) {
            tri |= (uint32_t)(unsigned char)file->sourcemap[i + j] << (16 - j * 8);
            bytes++;
        }
        encoded[elen++] = b64chars[(tri >> 18) & 0x3F];
        encoded[elen++] = b64chars[(tri >> 12) & 0x3F];
        if (bytes > 1) encoded[elen++] = b64chars[(tri >> 6) & 0x3F];
        if (bytes > 2) encoded[elen++] = b64chars[tri & 0x3F];
    }
    encoded[elen] = '\0';

    size_t cur = file->processed_len;
    if (cur + elen + 64 < sizeof(file->processed)) {
        snprintf(file->processed + cur, sizeof(file->processed) - cur,
            "\n/*# sourceMappingURL=data:application/json;base64,%s */\n", encoded);
        file->processed_len = strlen(file->processed);
    }
}

void ap_compute_content_hash(const char *data, size_t len, char out_hash[65]) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)(unsigned char)data[i];
        h *= 1099511628211ULL;
    }
    for (int i = 0; i < 16; i++) {
        snprintf(out_hash + i * 4, 5, "%04x", (unsigned)((h >> (i * 4)) & 0xFFFF));
    }
    out_hash[64] = '\0';
}

void ap_rename_with_hash(AssetPipeline *ap, APFile *file) {
    char hash[65];
    ap_compute_content_hash(file->processed, file->processed_len, hash);

    char hash_short[9];
    strncpy(hash_short, hash, 8);
    hash_short[8] = '\0';

    const char *base = strrchr(file->input_path, '/');
    if (!base) base = strrchr(file->input_path, '\\');
    if (!base) base = file->input_path;
    else base++;

    const char *dot = strrchr(base, '.');
    char name[256];
    char ext[32];
    if (dot) {
        size_t nlen = dot - base;
        strncpy(name, base, nlen < 255 ? nlen : 255);
        name[nlen < 255 ? nlen : 255] = '\0';
        strncpy(ext, dot, 31);
    } else {
        strncpy(name, base, 255);
        ext[0] = '\0';
    }

    snprintf(file->output_path, AP_MAX_PATH, "%s/%s.%s%s", ap->output_dir, name, hash_short, ext);
    strncpy(file->content_hash, hash, 64);
    file->content_hash[64] = '\0';
}

void ap_process_file(AssetPipeline *ap, int index) {
    if (index < 0 || index >= ap->file_count) return;
    APFile *f = &ap->files[index];
    f->status = AP_QUEUE_PROCESSING;

    FILE *in = fopen(f->input_path, "rb");
    if (!in) { f->status = AP_QUEUE_FAILED; return; }
    size_t r = fread(f->source, 1, sizeof(f->source) - 1, in);
    fclose(in);
    f->source[r] = '\0';
    f->source_len = r;

    const char *ext = strrchr(f->input_path, '.');
    if (ext) {
        if (strcmp(ext, ".scss") == 0 || strcmp(ext, ".sass") == 0) {
            if (ap->compile_sass) {
                ap_compile_sass(ap, f->source, f->source_len, f->processed, &f->processed_len);
            }
            if (ap->autoprefix) {
                char temp[131072];
                size_t temp_len;
                ap_autoprefix(ap, f->processed, f->processed_len, temp, &temp_len);
                memcpy(f->processed, temp, temp_len);
                f->processed_len = temp_len;
            }
        } else if (strcmp(ext, ".css") == 0) {
            if (ap->autoprefix) {
                ap_autoprefix(ap, f->source, f->source_len, f->processed, &f->processed_len);
            } else {
                memcpy(f->processed, f->source, f->source_len);
                f->processed_len = f->source_len;
            }
        } else if (strcmp(ext, ".png") == 0 || strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
            memcpy(f->processed, f->source, f->source_len);
            f->processed_len = f->source_len;
        } else {
            memcpy(f->processed, f->source, f->source_len);
            f->processed_len = f->source_len;
        }
    } else {
        memcpy(f->processed, f->source, f->source_len);
        f->processed_len = f->source_len;
    }

    if (ap->generate_sourcemaps) {
        ap_generate_sourcemap(ap, f->source, f->source_len, f->processed, f->processed_len,
            f->sourcemap, &f->sourcemap_len);
    }

    if (ap->content_hash) {
        ap_rename_with_hash(ap, f);
    } else {
        const char *base = strrchr(f->input_path, '/');
        if (!base) base = strrchr(f->input_path, '\\');
        if (!base) base = f->input_path; else base++;
        snprintf(f->output_path, AP_MAX_PATH, "%s/%s", ap->output_dir, base);
    }

    if (ap->content_hash && ap->generate_sourcemaps) {
        ap_sourcemap_base64_inline(ap, f);
    }

    FILE *out = fopen(f->output_path, "wb");
    if (out) {
        fwrite(f->processed, 1, f->processed_len, out);
        fclose(out);
        f->status = AP_QUEUE_DONE;
    } else {
        f->status = AP_QUEUE_FAILED;
    }
}

void ap_process_all(AssetPipeline *ap) {
    for (int i = 0; i < ap->file_count; i++) {
        ap_process_file(ap, i);
    }
}

void ap_print_report(AssetPipeline *ap) {
    printf("Asset Pipeline Report:\n");
    printf("  Input dir:  %s\n", ap->input_dir);
    printf("  Output dir: %s\n", ap->output_dir);
    printf("  Files processed: %d\n", ap->file_count);
    int done = 0, failed = 0, pending = 0;
    size_t total_in = 0, total_out = 0;
    for (int i = 0; i < ap->file_count; i++) {
        APFile *f = &ap->files[i];
        switch (f->status) {
            case AP_QUEUE_DONE:   done++; break;
            case AP_QUEUE_FAILED: failed++; break;
            default:              pending++; break;
        }
        total_in += f->source_len;
        total_out += f->processed_len;
    }
    printf("  Done: %d, Failed: %d, Pending: %d\n", done, failed, pending);
    printf("  Input size:  %zu bytes\n", total_in);
    printf("  Output size: %zu bytes\n", total_out);
    printf("  SASS variables: %d\n", ap->sass_ctx.var_count);
    printf("  Prefix rules: %d\n", ap->prefix_count);

    for (int i = 0; i < ap->file_count; i++) {
        APFile *f = &ap->files[i];
        const char *status_str = "pending";
        if (f->status == AP_QUEUE_DONE) status_str = "done";
        else if (f->status == AP_QUEUE_FAILED) status_str = "failed";
        printf("  [%s] %s -> %s (%zu -> %zu bytes)\n",
            status_str, f->input_path, f->output_path, f->source_len, f->processed_len);
        if (f->content_hash[0])
            printf("      hash: %.8s...\n", f->content_hash);
    }
}
