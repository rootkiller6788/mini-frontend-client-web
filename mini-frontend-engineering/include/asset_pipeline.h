#ifndef ASSET_PIPELINE_H
#define ASSET_PIPELINE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define AP_MAX_FILES            1024
#define AP_MAX_PATH             512
#define AP_SASS_NEST_DEPTH      32
#define AP_BROWSER_QUERY_COUNT  64
#define AP_MEDIA_QUERY_COUNT    32
#define AP_PREFIX_COUNT         128

typedef enum {
    AP_IMAGE_PNG,
    AP_IMAGE_JPEG,
    AP_IMAGE_WEBP,
    AP_IMAGE_AVIF,
    AP_IMAGE_SVG,
    AP_IMAGE_GIF
} APImageFormat;

typedef enum {
    AP_RESIZE_CONTAIN,
    AP_RESIZE_COVER,
    AP_RESIZE_FILL,
    AP_RESIZE_NONE
} APResizeMode;

typedef enum {
    AP_QUEUE_PENDING,
    AP_QUEUE_PROCESSING,
    AP_QUEUE_DONE,
    AP_QUEUE_FAILED
} APQueueStatus;

typedef struct {
    char input_path[AP_MAX_PATH];
    char output_path[AP_MAX_PATH];
    APImageFormat format;
    int width;
    int height;
    int quality;
    APResizeMode resize_mode;
    double scale_factor;
    bool generate_srcset;
    int srcset_widths[8];
    int srcset_count;
} APImageConfig;

typedef struct {
    char variable_name[128];
    char variable_value[256];
    int scope_depth;
    bool is_global;
} APSassVariable;

typedef struct {
    char name[128];
    APSassVariable variables[64];
    int var_count;
    char css_output[65536];
    size_t css_output_len;
    int nest_depth;
    bool in_at_rule;
    char current_selector[512];
} APSassContext;

typedef struct {
    char property[128];
    char value[256];
    bool needs_webkit;
    bool needs_moz;
    bool needs_ms;
    bool needs_o;
} APPrefixRule;

typedef struct {
    char input_path[AP_MAX_PATH];
    char output_path[AP_MAX_PATH];
    char source[65536];
    size_t source_len;
    char processed[131072];
    size_t processed_len;
    char sourcemap[262144];
    size_t sourcemap_len;
    char content_hash[65];
    APQueueStatus status;
} APFile;

typedef struct {
    char input_dir[AP_MAX_PATH];
    char output_dir[AP_MAX_PATH];
    char public_path[256];
    bool optimize_images;
    bool compile_sass;
    bool autoprefix;
    bool generate_sourcemaps;
    bool content_hash;
    bool inline_small;
    int inline_threshold_bytes;
    APSassContext sass_ctx;
    APPrefixRule prefixes[AP_PREFIX_COUNT];
    int prefix_count;
    APFile files[AP_MAX_FILES];
    int file_count;
} AssetPipeline;

void ap_init(AssetPipeline *ap, const char *input_dir, const char *output_dir);
int  ap_add_file(AssetPipeline *ap, const char *path);
void ap_set_image_config(AssetPipeline *ap, const char *path, APImageFormat fmt, int w, int h, int quality);
void ap_image_resize(const char *in_path, const char *out_path, int w, int h, APResizeMode mode);
void ap_image_convert_format(const char *in_path, const char *out_path, APImageFormat fmt, int quality);
void ap_generate_srcset(AssetPipeline *ap, APImageConfig *cfg);
int  ap_compile_sass(AssetPipeline *ap, const char *scss_str, size_t len, char *out_css, size_t *out_len);
void ap_sass_define_variable(APSassContext *ctx, const char *name, const char *value, bool global);
void ap_sass_resolve_nesting(APSassContext *ctx, char *scss, size_t *len);
void ap_sass_resolve_variables(APSassContext *ctx, char *scss, size_t *len);
int  ap_autoprefix(AssetPipeline *ap, const char *css_str, size_t len, char *out_css, size_t *out_len);
void ap_register_prefix(AssetPipeline *ap, const char *property, bool webkit, bool moz, bool ms, bool o);
void ap_generate_sourcemap(AssetPipeline *ap, const char *original, size_t orig_len, const char *processed, size_t proc_len, char *out_map, size_t *map_len);
void ap_sourcemap_base64_inline(AssetPipeline *ap, APFile *file);
void ap_compute_content_hash(const char *data, size_t len, char out_hash[65]);
void ap_rename_with_hash(AssetPipeline *ap, APFile *file);
void ap_process_all(AssetPipeline *ap);
void ap_process_file(AssetPipeline *ap, int index);
void ap_print_report(AssetPipeline *ap);

#endif
