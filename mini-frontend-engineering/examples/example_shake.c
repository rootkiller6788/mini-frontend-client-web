#include "tree_shaker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("=== Tree Shaker Example ===\n\n");

    TreeShaker ts;
    shaker_init(&ts);

    const char *math_src =
        "export function add(a, b) { return a + b; }\n"
        "export function sub(a, b) { return a - b; }\n"
        "export function mul(a, b) { return a * b; }\n"
        "export function div(a, b) { return a / b; }\n"
        "export const PI = 3.14159;\n"
        "function unusedHelper() { console.log('never called'); }\n";

    const char *utils_src =
        "import { add } from './math';\n"
        "export function format(template, value) { return template.replace('%d', value); }\n"
        "export function capitalize(str) { return str.charAt(0).toUpperCase() + str.slice(1); }\n";

    const char *index_src =
        "import { format } from './utils';\n"
        "function main() { console.log(format('Hello %s', 'World')); }\n"
        "main();\n";

    shaker_add_module(&ts, "math.js", math_src, strlen(math_src));
    shaker_add_module(&ts, "utils.js", utils_src, strlen(utils_src));
    shaker_add_module(&ts, "index.js", index_src, strlen(index_src));

    int math_idx = 0;
    int utils_idx = 1;

    ShakerModule *math = &ts.modules[math_idx];
    ShakerModule *utils = &ts.modules[utils_idx];

    math->ast = shaker_parse_source(math->source, math->source_len);
    utils->ast = shaker_parse_source(utils->source, utils->source_len);

    strcpy(math->dependencies[0], "none");
    math->dependency_count = 1;
    strcpy(utils->dependencies[0], "math.js");
    utils->dependency_count = 1;

    shaker_set_entry(&ts, "index.js");
    ts.modules[2].is_reachable = true;
    strcpy(ts.modules[2].dependencies[0], "utils.js");
    ts.modules[2].dependency_count = 1;

    printf("Module dependencies:\n");
    printf("  index.js -> utils.js\n");
    printf("  utils.js -> math.js\n");

    printf("\nAnalyzing exports...\n");
    shaker_analyze_exports(&ts, math);
    shaker_analyze_exports(&ts, utils);
    printf("  math.js exports: %d\n", math->export_count);
    for (int i = 0; i < math->export_count; i++)
        printf("    - %s\n", math->exports[i].name);
    printf("  utils.js exports: %d\n", utils->export_count);
    for (int i = 0; i < utils->export_count; i++)
        printf("    - %s\n", utils->exports[i].name);

    printf("\nMarking reachable modules...\n");
    shaker_mark_reachable(&ts);
    for (int i = 0; i < ts.module_count; i++)
        printf("  %s: reachable=%d\n", ts.modules[i].module_path, ts.modules[i].is_reachable);

    printf("\nMarking used exports...\n");
    shaker_mark_used_exports(&ts);
    for (int i = 0; i < math->export_count; i++)
        printf("  math.%s: used=%d\n", math->exports[i].name, math->exports[i].is_used);

    printf("\nEliminating dead code...\n");
    shaker_eliminate_dead_code(&ts);

    printf("\nScope hoisting (concatenate)...\n");
    shaker_scope_hoisting(&ts);

    shaker_print_stats(&ts);

    printf("\nStripped output:\n");
    for (int i = 0; i < ts.module_count; i++) {
        ShakerModule *m = &ts.modules[i];
        if (m->stripped_len > 0)
            printf("--- %s ---\n%s\n", m->module_path, m->stripped_source);
    }

    shaker_free_ast(math->ast);
    shaker_free_ast(utils->ast);

    printf("\nDone.\n");
    return 0;
}
