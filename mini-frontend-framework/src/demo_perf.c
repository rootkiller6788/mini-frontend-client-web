#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "virtual_dom.h"
#include "component_model.h"
#include "react_hooks.h"

#define PERF_ITERATIONS     1000
#define PERF_TREE_DEPTH     5
#define PERF_BRANCH_FACTOR  3
#define PERF_BENCH_RUNS     10

typedef struct {
    double create_time_ms;
    double diff_time_ms;
    double patch_time_ms;
    double clone_time_ms;
    double total_nodes;
    double nodes_per_ms;
} BenchmarkResult;

static VNode* create_deep_tree(int depth, int* counter) {
    VNode* node = vdom_create_element("div");
    char key[64];
    snprintf(key, sizeof(key), "node-%d", (*counter)++);
    vdom_set_key(node, key);
    vdom_set_prop(node, "class", "tree-node");

    if (depth > 0) {
        for (int i = 0; i < PERF_BRANCH_FACTOR; i++) {
            VNode* child = create_deep_tree(depth - 1, counter);
            vdom_append_child(node, child);
        }
    } else {
        VNode* text = vdom_create_text("leaf node");
        vdom_append_child(node, text);
    }
    return node;
}

static int count_nodes(VNode* node) {
    if (!node) return 0;
    int count = 1;
    for (int i = 0; i < node->child_count; i++) {
        count += count_nodes(node->children[i]);
    }
    return count;
}

static VNode* mutate_tree(VNode* original, int* counter) {
    VNode* node = vdom_create_element("div");
    char key[64];
    snprintf(key, sizeof(key), "node-%d", (*counter)++);
    vdom_set_key(node, key);
    vdom_set_prop(node, "class", "tree-node-updated");

    if (original->child_count > 0) {
        for (int i = 0; i < original->child_count; i++) {
            VNode* child = mutate_tree(original->children[i], counter);
            vdom_append_child(node, child);
        }
    } else {
        VNode* text = vdom_create_text("updated leaf");
        vdom_append_child(node, text);
    }
    return node;
}

static double get_time_ms(clock_t start, clock_t end) {
    return ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
}

static BenchmarkResult run_benchmark(void) {
    BenchmarkResult result = {0};
    double total_create = 0, total_diff = 0, total_patch = 0, total_clone = 0;
    long total_nodes = 0;

    for (int run = 0; run < PERF_BENCH_RUNS; run++) {
        int counter = 0;

        clock_t t1 = clock();
        VNode* tree = create_deep_tree(PERF_TREE_DEPTH, &counter);
        clock_t t2 = clock();
        total_create += get_time_ms(t1, t2);
        total_nodes += count_nodes(tree);

        counter = 0;
        VNode* mutated = mutate_tree(tree, &counter);

        clock_t t3 = clock();
        vdom_diff(tree, mutated, NULL);
        clock_t t4 = clock();
        total_diff += get_time_ms(t3, t4);

        clock_t t5 = clock();
        vdom_patch(tree, mutated);
        clock_t t6 = clock();
        total_patch += get_time_ms(t5, t6);

        clock_t t7 = clock();
        VNode* cloned = vdom_clone(tree);
        clock_t t8 = clock();
        total_clone += get_time_ms(t7, t8);

        vdom_free_recursive(cloned);
        vdom_free_recursive(mutated);
        vdom_free_recursive(tree);
    }

    result.create_time_ms = total_create / PERF_BENCH_RUNS;
    result.diff_time_ms = total_diff / PERF_BENCH_RUNS;
    result.patch_time_ms = total_patch / PERF_BENCH_RUNS;
    result.clone_time_ms = total_clone / PERF_BENCH_RUNS;
    result.total_nodes = (double)total_nodes / PERF_BENCH_RUNS;
    result.nodes_per_ms = result.total_nodes / result.create_time_ms;

    return result;
}

static void demo_keyed_diff_scenario(void) {
    printf("\n--- Keyed Diff Scenario ---\n");

    VNode* old_list = vdom_create_element("ul");
    for (int i = 0; i < 5; i++) {
        VNode* li = vdom_create_element("li");
        char key[16];
        snprintf(key, sizeof(key), "item-%d", i);
        vdom_set_key(li, key);
        char text[32];
        snprintf(text, sizeof(text), "Item %d (old)", i);
        vdom_set_text(li, text);
        vdom_append_child(old_list, li);
    }

    printf("  Old list (5 items, keyed):\n");
    vdom_debug_print(old_list, 1);

    VNode* new_list = vdom_create_element("ul");
    int new_order[] = {0, 4, 2, 1, 3, 5};
    for (int i = 0; i < 6; i++) {
        int idx = new_order[i];
        VNode* li = vdom_create_element("li");
        char key[16];
        snprintf(key, sizeof(key), "item-%d", idx);
        vdom_set_key(li, key);
        char text[32];
        if (idx == 5)
            snprintf(text, sizeof(text), "Item %d (NEW)", idx);
        else if (idx == 1)
            snprintf(text, sizeof(text), "Item %d (updated)", idx);
        else
            snprintf(text, sizeof(text), "Item %d", idx);
        vdom_set_text(li, text);
        vdom_append_child(new_list, li);
    }

    printf("  New list (6 items, reordered + 1 new):\n");
    vdom_debug_print(new_list, 1);

    printf("  Running keyed reconciliation...\n");

    VNodeList old_nodes, new_nodes;
    vdom_node_list_init(&old_nodes, old_list->child_count);
    vdom_node_list_init(&new_nodes, new_list->child_count);
    for (int i = 0; i < old_list->child_count; i++)
        vdom_node_list_add(&old_nodes, old_list->children[i]);
    for (int i = 0; i < new_list->child_count; i++)
        vdom_node_list_add(&new_nodes, new_list->children[i]);

    vdom_keyed_reconciliation(&old_nodes, &new_nodes, NULL, old_list);

    vdom_node_list_free(&old_nodes);
    vdom_node_list_free(&new_nodes);
    printf("  Reconciliation complete\n");

    vdom_free_recursive(old_list);
    vdom_free_recursive(new_list);
}

static void demo_batch_throughput(void) {
    printf("\n--- Batch Throughput Test ---\n");

    vdom_batch_begin();

    VNode* root = vdom_create_element("div");
    for (int i = 0; i < 100; i++) {
        VNode* child = vdom_create_element("span");
        char text[32];
        snprintf(text, sizeof(text), "Batch item %d", i);
        vdom_set_text(child, text);
        vdom_append_child(root, child);
    }

    printf("  Created 100 nodes in batch mode\n");
    printf("  Batch pending: %s\n", vdom_batch_is_pending() ? "yes" : "no");

    vdom_batch_end();
    printf("  Batch flushed\n");

    vdom_free_recursive(root);
}

static void demo_component_render_throughput(void) {
    printf("\n--- Component Render Throughput ---\n");

    for (int i = 0; i < 10; i++) {
        Component* comp = comp_create((char[]){"Comp"}, NULL);
        char name[32];
        snprintf(name, sizeof(name), "dynamic-comp-%d", i);
        strncpy(comp->name, name, sizeof(comp->name) - 1);
        comp_free(comp);
    }
    printf("  10 components created/destroyed (memory stress test)\n");
}

static void demo_memory_footprint(void) {
    printf("\n--- Memory Footprint Analysis ---\n");
    printf("  sizeof(VNode)       = %zu bytes\n", sizeof(VNode));
    printf("  sizeof(VProp)       = %zu bytes\n", sizeof(VProp));
    printf("  sizeof(VPatch)      = %zu bytes\n", sizeof(VPatch));
    printf("  sizeof(Component)   = %zu bytes\n", sizeof(Component));
    printf("  sizeof(Hook)        = %zu bytes\n", sizeof(Hook));
    printf("  sizeof(HookState)   = %zu bytes\n", sizeof(HookState));
    printf("  sizeof(Router)      = %zu bytes\n", sizeof(Router));
    printf("  sizeof(Route)       = %zu bytes\n", sizeof(Route));
    printf("  sizeof(Fiber)       = %zu bytes\n", sizeof(Fiber));

    size_t vnode_cost = sizeof(VNode) + VDOM_MAX_CHILDREN * sizeof(VNode*) + VDOM_MAX_PROPS * sizeof(VProp);
    printf("\n  Max VNode memory: %zu bytes (%.2f KB)\n", vnode_cost, vnode_cost / 1024.0);

    int hypothetical_nodes = 1000;
    size_t tree_cost = hypothetical_nodes * vnode_cost;
    printf("  1000-node tree: %zu bytes (%.2f MB)\n", tree_cost, tree_cost / (1024.0 * 1024.0));
}

int main(void) {
    printf("========================================\n");
    printf("  Mini Frontend Framework - Performance Demo\n");
    printf("========================================\n");

    printf("\n[Benchmark Configuration]\n");
    printf("  Tree depth: %d\n", PERF_TREE_DEPTH);
    printf("  Branch factor: %d\n", PERF_BRANCH_FACTOR);
    printf("  Benchmark runs: %d\n", PERF_BENCH_RUNS);

    printf("\n[1] Running VDOM benchmarks...\n");
    BenchmarkResult result = run_benchmark();

    printf("\n[Results]\n");
    printf("  Average tree size:    %.0f nodes\n", result.total_nodes);
    printf("  Tree creation:        %.3f ms (%.1f nodes/ms)\n",
           result.create_time_ms, result.nodes_per_ms);
    printf("  Diff algorithm:       %.3f ms\n", result.diff_time_ms);
    printf("  Patch application:    %.3f ms\n", result.patch_time_ms);
    printf("  Tree cloning:         %.3f ms\n", result.clone_time_ms);

    double total_op = result.create_time_ms + result.diff_time_ms +
                      result.patch_time_ms + result.clone_time_ms;
    printf("  Total benchmark time: %.3f ms\n", total_op);

    printf("\n[2] Individual operation micro-benchmarks...\n");

    clock_t t1 = clock();
    for (int i = 0; i < PERF_ITERATIONS; i++) {
        VNode* el = vdom_create_element("div");
        vdom_set_prop(el, "class", "test");
        vdom_set_text(el, "hello");
        vdom_free(el);
    }
    clock_t t2 = clock();
    double el_time = get_time_ms(t1, t2);
    printf("  %d create/set/free cycles: %.3f ms (%.1f ops/ms)\n",
           PERF_ITERATIONS, el_time, PERF_ITERATIONS / el_time);

    VNode* parent = vdom_create_element("div");
    t1 = clock();
    for (int i = 0; i < PERF_ITERATIONS; i++) {
        VNode* child = vdom_create_text("child");
        vdom_append_child(parent, child);
        vdom_remove_child(parent, child);
        vdom_free(child);
    }
    t2 = clock();
    vdom_free(parent);
    double append_time = get_time_ms(t1, t2);
    printf("  %d append/remove cycles: %.3f ms (%.1f ops/ms)\n",
           PERF_ITERATIONS, append_time, PERF_ITERATIONS / append_time);

    VNode* a = vdom_create_element("div");
    vdom_set_text(a, "hello world from C99 VDOM");
    t1 = clock();
    for (int i = 0; i < PERF_ITERATIONS; i++) {
        VNode* b = vdom_clone(a);
        vdom_free_recursive(b);
    }
    t2 = clock();
    vdom_free_recursive(a);
    double clone_time = get_time_ms(t1, t2);
    printf("  %d clone/free cycles: %.3f ms (%.1f ops/ms)\n",
           PERF_ITERATIONS, clone_time, PERF_ITERATIONS / clone_time);

    t1 = clock();
    for (int i = 0; i < PERF_ITERATIONS; i++) {
        VNode* old_node = vdom_create_element("span");
        vdom_set_text(old_node, "old");
        VNode* new_node = vdom_create_element("span");
        vdom_set_text(new_node, "new");
        vdom_diff(old_node, new_node, NULL);
        vdom_free(old_node);
        vdom_free(new_node);
    }
    t2 = clock();
    double diff_time = get_time_ms(t1, t2);
    printf("  %d diff cycles: %.3f ms (%.1f ops/ms)\n",
           PERF_ITERATIONS, diff_time, PERF_ITERATIONS / diff_time);

    printf("\n[3] Running scenario-based tests...\n");
    demo_keyed_diff_scenario();
    demo_batch_throughput();
    demo_component_render_throughput();

    printf("\n[4] Memory analysis...\n");
    demo_memory_footprint();

    printf("\n[5] Hook state performance...\n");
    {
        HookState* hs = hooks_create_state(NULL, NULL);
        t1 = clock();
        for (int i = 0; i < PERF_ITERATIONS; i++) {
            hooks_begin_render(hs);
            void* val = NULL;
            int slot = hooks_use_state(hs, NULL, &val);
            if (!val) {
                static int v = 0;
                hooks_set_state(hs, slot, &v);
            }
            hooks_end_render(hs);
        }
        t2 = clock();
        double hook_time = get_time_ms(t1, t2);
        printf("  %d hook cycles: %.3f ms (%.1f ops/ms)\n",
               PERF_ITERATIONS, hook_time, PERF_ITERATIONS / hook_time);
        hooks_destroy_state(hs);
    }

    printf("\n[6] Stress test: large tree operations...\n");
    {
        int counter = 0;
        t1 = clock();
        VNode* deep_tree = create_deep_tree(6, &counter);
        t2 = clock();
        double deep_create = get_time_ms(t1, t2);
        int node_count = count_nodes(deep_tree);
        printf("  Deep tree (depth=6): %d nodes created in %.3f ms\n", node_count, deep_create);

        vdom_free_recursive(deep_tree);
    }

    printf("\n========================================\n");
    printf("  Performance Demo Completed\n");
    printf("========================================\n");
    return 0;
}
