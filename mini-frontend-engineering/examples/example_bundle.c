#include "bundler_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void create_test_files(void) {
    FILE *f;

    f = fopen("example_src/index.js", "wb");
    fprintf(f, "import { add } from './math';\n"
               "import { format } from './utils';\n"
               "import lodash from 'lodash';\n\n"
               "function main() {\n"
               "  const result = add(1, 2);\n"
               "  console.log(format('Result: %d', result));\n"
               "  return lodash.cloneDeep({ result });\n"
               "}\n\n"
               "export default main;\n");
    fclose(f);

    f = fopen("example_src/math.js", "wb");
    fprintf(f, "export function add(a, b) { return a + b; }\n"
               "export function sub(a, b) { return a - b; }\n"
               "export function mul(a, b) { return a * b; }\n"
               "export function div(a, b) { return a / b; }\n");
    fclose(f);

    f = fopen("example_src/utils.js", "wb");
    fprintf(f, "export function format(template, value) {\n"
               "  return template.replace('%%d', value);\n"
               "}\n");
    fclose(f);

    f = fopen("example_src/node_modules/lodash/package.json", "wb");
    fprintf(f, "{\"name\":\"lodash\",\"version\":\"4.17.21\",\"main\":\"index.js\"}\n");
    fclose(f);

    f = fopen("example_src/node_modules/lodash/index.js", "wb");
    fprintf(f, "module.exports = {\n"
               "  cloneDeep: function(obj) { return JSON.parse(JSON.stringify(obj)); }\n"
               "};\n");
    fclose(f);
}

int main(void) {
    printf("=== Mini Bundler Example ===\n\n");

    create_test_files();

    Bundler b;
    bundler_init(&b, "example_src");

    bundler_add_entry(&b, "example_src/index.js");

    printf("Building dependency graph...\n");
    bundler_build_dependency_graph(&b);

    printf("Splitting chunks (vendor/app)...\n");
    bundler_split_chunks(&b);

    printf("Generating bundle output...\n");
    bundler_generate_bundle(&b);

    bundler_print_stats(&b);

    printf("\nWriting output to dist/...\n");
    bundler_write_output(&b, "dist");

    printf("\nResolving modules:\n");
    char resolved[MAX_PATH_LEN];
    if (bundler_resolve_module(&b, "example_src/index.js", "./math", resolved, sizeof(resolved)) == 0)
        printf("  './math' -> %s\n", resolved);
    if (bundler_resolve_module(&b, "example_src/index.js", "./utils", resolved, sizeof(resolved)) == 0)
        printf("  './utils' -> %s\n", resolved);

    printf("\nDone. Check dist/ for output bundles.\n");
    return 0;
}
