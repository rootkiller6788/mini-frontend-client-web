#include "asset_pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void create_test_assets(void) {
    FILE *f;

    f = fopen("assets/styles.scss", "wb");
    fprintf(f,
        "$primary-color: #1976d2;\n"
        "$border-radius: 4px;\n\n"
        ".container {\n"
        "  display: flex;\n"
        "  justify-content: center;\n"
        "  .item {\n"
        "    color: $primary-color;\n"
        "    border-radius: $border-radius;\n"
        "    transform: scale(1);\n"
        "    transition: all 0.3s ease;\n"
        "    user-select: none;\n"
        "    &:hover {\n"
        "      transform: scale(1.1);\n"
        "      box-shadow: 0 2px 8px rgba(0,0,0,0.2);\n"
        "    }\n"
        "  }\n"
        "}\n\n"
        ".header {\n"
        "  animation: fadeIn 0.5s;\n"
        "  backface-visibility: hidden;\n"
        "  column-count: 2;\n"
        "  appearance: none;\n"
        "}\n");
    fclose(f);

    f = fopen("assets/main.css", "wb");
    fprintf(f,
        "body {\n"
        "  margin: 0;\n"
        "  font-family: sans-serif;\n"
        "}\n"
        ".header { animation: fadeIn 0.5s; }\n"
        "@keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }\n");
    fclose(f);

    f = fopen("assets/logo.png", "wb");
    fwrite("FAKE_PNG_DATA_1234567890", 1, 23, f);
    fclose(f);

    f = fopen("assets/banner.jpg", "wb");
    fwrite("FAKE_JPEG_DATA_9876543210", 1, 24, f);
    fclose(f);
}

int main(void) {
    printf("=== Asset Pipeline Example ===\n\n");

    create_test_assets();

    AssetPipeline ap;
    ap_init(&ap, "assets", "dist_assets");

    ap_add_file(&ap, "assets/styles.scss");
    ap_add_file(&ap, "assets/main.css");
    ap_add_file(&ap, "assets/logo.png");
    ap_add_file(&ap, "assets/banner.jpg");

    printf("Processing files...\n\n");
    ap_process_all(&ap);

    printf("Report:\n");
    ap_print_report(&ap);

    printf("\n--- SASS Compilation Demo ---\n");
    const char *test_scss =
        "$primary: red;\n"
        ".btn {\n"
        "  color: $primary;\n"
        "  &--large { font-size: 20px; }\n"
        "}\n";

    char css_out[65536];
    size_t css_len = 0;
    ap_compile_sass(&ap, test_scss, strlen(test_scss), css_out, &css_len);
    printf("SCSS input:\n%s\n", test_scss);
    printf("CSS output:\n%s\n", css_out);

    printf("\n--- Autoprefixer Demo ---\n");
    const char *test_css =
        ".box {\n"
        "  display: flex;\n"
        "  transform: rotate(45deg);\n"
        "  transition: all 0.3s;\n"
        "}\n";

    char prefixed[65536];
    size_t prefixed_len = 0;
    ap_autoprefix(&ap, test_css, strlen(test_css), prefixed, &prefixed_len);
    printf("CSS input:\n%s\n", test_css);
    printf("Prefixed output:\n%s\n", prefixed);

    printf("\n--- Content Hash Demo ---\n");
    char hash[65];
    ap_compute_content_hash("hello, world!", 13, hash);
    printf("Hash of 'hello, world!': %s\n", hash);

    ap_compute_content_hash("hello, world", 12, hash);
    printf("Hash of 'hello, world': %s\n", hash);

    printf("\n--- Image Resize Demo ---\n");
    ap_image_resize("assets/logo.png", "dist_assets/logo_64.png", 64, 64, AP_RESIZE_COVER);
    printf("Resized: logo.png -> dist_assets/logo_64.png (64x64)\n");

    printf("\n--- Sourcemap Generation ---\n");
    char map[262144];
    size_t map_len;
    ap_generate_sourcemap(&ap, test_css, strlen(test_css), prefixed, prefixed_len, map, &map_len);
    printf("Sourcemap (%zu bytes):\n%.200s...\n", map_len, map);

    printf("\nDone. Check dist_assets/ for output.\n");
    return 0;
}
