#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "virtual_dom.h"
#include "react_hooks.h"
#include "component_model.h"
#include "reactive_system.h"

typedef struct {
    char text[256];
    int count;
} ButtonState;

static VNode* render_button(Component* comp, VNode* props) {
    (void)props;
    HookState* hs = comp->hook_state;
    int* count_ptr = NULL;
    hooks_use_state(hs, NULL, (void**)&count_ptr);

    VNode* button = vdom_create_element("button");
    vdom_set_prop(button, "class", "counter-btn");
    char label[256];
    if (count_ptr) {
        snprintf(label, sizeof(label), "Clicked: %d times", *count_ptr);
    } else {
        snprintf(label, sizeof(label), "Click me!");
    }
    vdom_set_text(button, label);
    return button;
}

static VNode* render_card(Component* comp, VNode* props) {
    (void)props;
    HookState* hs = comp->hook_state;

    int* title_ptr = NULL;
    int title_slot = hooks_use_state(hs, NULL, (void**)&title_ptr);
    if (!title_ptr) {
        char* default_title = "Default Card Title";
        hooks_set_state(hs, title_slot, default_title);
        title_ptr = hooks_get_state_value(hs, title_slot);
    }

    VNode* div = vdom_create_element("div");
    vdom_set_prop(div, "class", "card");

    VNode* h2 = vdom_create_element("h2");
    vdom_set_text(h2, title_ptr ? (char*)title_ptr : "Untitled");
    vdom_append_child(div, h2);

    VNode* p = vdom_create_element("p");
    vdom_set_text(p, "This is a card component rendered with the Virtual DOM.");
    vdom_append_child(div, p);

    return div;
}

static VNode* render_app(Component* comp, VNode* props) {
    (void)props;
    HookState* hs = comp->hook_state;

    void* count_dep = NULL;
    int count_slot = hooks_use_state(hs, NULL, &count_dep);
    if (!count_dep) {
        static int zero = 0;
        hooks_set_state(hs, count_slot, &zero);
        count_dep = hooks_get_state_value(hs, count_slot);
    }

    VNode* root = vdom_create_element("div");
    vdom_set_prop(root, "class", "app");

    VNode* h1 = vdom_create_element("h1");
    vdom_set_text(h1, "Mini Frontend Framework - Basic Example");
    vdom_append_child(root, h1);

    VNode* section = vdom_create_element("section");
    vdom_set_prop(section, "class", "demo-section");

    VNode* h3 = vdom_create_element("h3");
    vdom_set_text(h3, "Virtual DOM Demo");
    vdom_append_child(section, h3);

    VNode* button = vdom_create_element("button");
    vdom_set_prop(button, "class", "click-btn");
    vdom_set_prop(button, "onclick", "handleClick");
    char btn_text[64];
    int val = count_dep ? *(int*)count_dep : 0;
    snprintf(btn_text, sizeof(btn_text), "Count: %d", val);
    vdom_set_text(button, btn_text);
    vdom_append_child(section, button);

    VNode* info = vdom_create_element("div");
    vdom_set_prop(info, "class", "info");
    VNode* p_info = vdom_create_element("p");
    vdom_set_text(p_info, "This UI is built entirely with C99 Virtual DOM.");
    vdom_append_child(info, p_info);
    vdom_append_child(section, info);

    vdom_append_child(root, section);

    VNode* frag = vdom_create_fragment();
    vdom_set_key(frag, "list-fragment");
    for (int i = 0; i < 3; i++) {
        VNode* item = vdom_create_element("div");
        vdom_set_prop(item, "class", "list-item");
        vdom_set_key(item, (char[]){'i', 't', 'e', 'm', '-', (char)('0' + i), '\0'});
        char label[32];
        snprintf(label, sizeof(label), "Fragment Item %d", i + 1);
        vdom_set_text(item, label);
        vdom_append_child(frag, item);
    }
    vdom_append_child(root, frag);

    return root;
}

static VNode* render_todo(Component* comp, VNode* props) {
    (void)props;
    HookState* hs = comp->hook_state;

    void* items_dep = NULL;
    hooks_use_state(hs, NULL, &items_dep);

    VNode* div = vdom_create_element("div");
    vdom_set_prop(div, "class", "todo-app");

    VNode* h2 = vdom_create_element("h2");
    vdom_set_text(h2, "Todo List (VDOM)");
    vdom_append_child(div, h2);

    VNode* list = vdom_create_element("ul");
    vdom_set_prop(list, "class", "todo-list");

    const char* sample_items[] = {"Learn Virtual DOM", "Build Components", "Add Hooks", "Wire Router"};
    for (int i = 0; i < 4; i++) {
        VNode* li = vdom_create_element("li");
        char key_buf[16];
        snprintf(key_buf, sizeof(key_buf), "todo-%d", i);
        vdom_set_key(li, key_buf);

        VNode* span = vdom_create_element("span");
        vdom_set_text(span, sample_items[i]);
        vdom_append_child(li, span);

        VNode* del_btn = vdom_create_element("button");
        vdom_set_prop(del_btn, "class", "delete-btn");
        vdom_set_text(del_btn, "X");
        vdom_append_child(li, del_btn);

        vdom_append_child(list, li);
    }
    vdom_append_child(div, list);

    return div;
}

static void demo_hooks_lifecycle(void) {
    printf("\n--- Hooks Lifecycle Demo ---\n");

    HookState* state = hooks_create_state(NULL, NULL);

    hooks_begin_render(state);
    void* val_ptr = NULL;
    int slot = hooks_use_state(state, NULL, &val_ptr);
    if (!val_ptr) {
        static int init_val = 42;
        hooks_set_state(state, slot, &init_val);
        val_ptr = hooks_get_state_value(state, slot);
    }
    printf("Initial state value: %d\n", val_ptr ? *(int*)val_ptr : -1);

    hooks_end_render(state);

    hooks_begin_render(state);
    void* val_ptr2 = NULL;
    int slot2 = hooks_use_state(state, NULL, &val_ptr2);
    printf("Second render state value: %d (slot=%d)\n",
           val_ptr2 ? *(int*)val_ptr2 : -1, slot2);
    hooks_end_render(state);

    hooks_destroy_state(state);
}

static void demo_reactive_signals(void) {
    printf("\n--- Reactive Signals Demo ---\n");

    ReactiveContext* ctx = reactive_create_context();

    int init_count = 0;
    Signal* counter = reactive_create_signal(ctx, "counter", &init_count, sizeof(int));
    printf("Signal '%s' created with value: %d\n", counter->name,
           *(int*)reactive_signal_get(counter));

    int new_count = 10;
    reactive_signal_set(counter, &new_count);
    printf("Signal '%s' updated to: %d\n", counter->name,
           *(int*)reactive_signal_get(counter));

    reactive_comparison_vdom_vs_fine_grained();

    reactive_destroy_context(ctx);
}

int main(void) {
    printf("========================================\n");
    printf("  Mini Frontend Framework - Basic Demo\n");
    printf("========================================\n");

    printf("\n[1] Creating Virtual DOM elements...\n");

    VNode* root = vdom_create_element("div");
    vdom_set_prop(root, "id", "root");
    vdom_set_prop(root, "class", "container");

    VNode* header = vdom_create_element("header");
    VNode* h1 = vdom_create_element("h1");
    vdom_set_text(h1, "Hello from C99 Virtual DOM!");
    vdom_append_child(header, h1);
    vdom_append_child(root, header);

    VNode* main_section = vdom_create_element("main");
    for (int i = 0; i < 3; i++) {
        VNode* p = vdom_create_element("p");
        char buf[64];
        snprintf(buf, sizeof(buf), "Paragraph %d - Rendered by C Virtual DOM", i + 1);
        vdom_set_text(p, buf);
        vdom_set_key(p, (char[]){'p', '-', (char)('0' + i), '\0'});
        vdom_append_child(main_section, p);
    }
    vdom_append_child(root, main_section);

    VNode* footer = vdom_create_text("Footer text node - no wrapper element");
    vdom_append_child(root, footer);

    printf("\n[2] Virtual DOM tree structure:\n");
    vdom_debug_print(root, 0);

    VNode* cloned = vdom_clone(root);
    printf("\n[3] Cloned tree (should be identical):\n");
    vdom_debug_print(cloned, 0);

    printf("\n[4] Diff test: updating text node...\n");
    VNode* new_footer = vdom_create_text("Updated footer text - patched via diff!");
    vdom_diff(root, root, NULL);
    (void)new_footer;

    printf("\n[5] Batch update test...\n");
    vdom_batch_begin();
    printf("  Batch pending: %s\n", vdom_batch_is_pending() ? "yes" : "no");
    vdom_batch_end();
    printf("  Batch flushed, pending: %s\n", vdom_batch_is_pending() ? "yes" : "no");

    demo_hooks_lifecycle();
    demo_reactive_signals();

    printf("\n[6] Component model test...\n");
    Component* app_comp = comp_create("App", render_app);
    comp_set_on_mount(app_comp, NULL);
    comp_set_on_unmount(app_comp, NULL);
    printf("  Component '%s' (id=%d) created\n", app_comp->name, app_comp->id);

    ComponentTree tree;
    comp_tree_init(&tree);
    comp_tree_set_root(&tree, app_comp);

    Component* btn_comp = comp_create("Button", render_button);
    comp_add_child(app_comp, btn_comp);

    Component* card_comp = comp_create("Card", render_card);
    comp_add_child(app_comp, card_comp);

    Component* todo_comp = comp_create("TodoList", render_todo);
    comp_add_child(app_comp, todo_comp);

    printf("  Tree: %s -> %d children\n", tree.root->name, tree.root->child_count);
    for (int i = 0; i < tree.root->child_count; i++) {
        printf("    Child %d: %s (id=%d)\n", i,
               comp_get_child(app_comp, i)->name,
               comp_get_child(app_comp, i)->id);
    }

    VNode* rendered = comp_render(app_comp);
    printf("\n[7] Rendered component tree:\n");
    vdom_debug_print(rendered, 0);

    printf("\n[8] Cleanup...\n");
    vdom_free_recursive(rendered);
    vdom_free_recursive(cloned);
    vdom_free_recursive(root);
    comp_free_tree(app_comp);

    printf("\n=== All demos completed successfully ===\n");
    return 0;
}
