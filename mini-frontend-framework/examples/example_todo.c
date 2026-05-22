#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "virtual_dom.h"
#include "react_hooks.h"
#include "component_model.h"

#define MAX_TODOS 64

typedef struct {
    int id;
    char text[128];
    bool completed;
} TodoItem;

typedef struct {
    TodoItem items[MAX_TODOS];
    int count;
    int next_id;
    char filter[16];
} TodoState;

static VNode* render_todo_item(int id, const char* text, bool completed) {
    VNode* li = vdom_create_element("li");
    char key[16];
    snprintf(key, sizeof(key), "todo-%d", id);
    vdom_set_key(li, key);
    vdom_set_prop(li, "class", completed ? "todo-item completed" : "todo-item");

    VNode* checkbox = vdom_create_element("input");
    vdom_set_prop(checkbox, "type", "checkbox");
    vdom_set_prop(checkbox, "checked", completed ? "true" : "false");
    char event_key[64];
    snprintf(event_key, sizeof(event_key), "onchange:todo:%d", id);
    vdom_set_event(checkbox, "onchange", event_key);
    vdom_append_child(li, checkbox);

    VNode* span = vdom_create_element("span");
    vdom_set_prop(span, "class", "todo-text");
    vdom_set_text(span, text);
    vdom_append_child(li, span);

    VNode* delete_btn = vdom_create_element("button");
    vdom_set_prop(delete_btn, "class", "btn-delete");
    vdom_set_text(delete_btn, "Delete");
    char del_event[64];
    snprintf(del_event, sizeof(del_event), "onclick:delete:%d", id);
    vdom_set_event(delete_btn, "onclick", del_event);
    vdom_append_child(li, delete_btn);

    return li;
}

static VNode* render_filter_bar(const char* current_filter) {
    VNode* div = vdom_create_element("div");
    vdom_set_prop(div, "class", "filter-bar");

    const char* filters[] = {"all", "active", "completed"};
    for (int i = 0; i < 3; i++) {
        VNode* btn = vdom_create_element("button");
        char btn_class[32];
        snprintf(btn_class, sizeof(btn_class), "filter-btn%s",
                 (strcmp(filters[i], current_filter) == 0) ? " active" : "");
        vdom_set_prop(btn, "class", btn_class);
        vdom_set_text(btn, filters[i]);
        char filter_event[32];
        snprintf(filter_event, sizeof(filter_event), "onclick:filter:%s", filters[i]);
        vdom_set_event(btn, "onclick", filter_event);
        vdom_append_child(div, btn);
    }
    return div;
}

static VNode* render_todo_stats(int total, int completed) {
    VNode* div = vdom_create_element("div");
    vdom_set_prop(div, "class", "todo-stats");

    char stats[128];
    snprintf(stats, sizeof(stats), "%d of %d items completed", completed, total);
    VNode* p = vdom_create_element("p");
    vdom_set_text(p, stats);
    vdom_append_child(div, p);

    return div;
}

static VNode* render_todo_input(void) {
    VNode* div = vdom_create_element("div");
    vdom_set_prop(div, "class", "todo-input-container");

    VNode* input = vdom_create_element("input");
    vdom_set_prop(input, "type", "text");
    vdom_set_prop(input, "class", "todo-input");
    vdom_set_prop(input, "placeholder", "What needs to be done?");
    vdom_set_prop(input, "id", "new-todo-input");
    vdom_append_child(div, input);

    VNode* add_btn = vdom_create_element("button");
    vdom_set_prop(add_btn, "class", "btn-add");
    vdom_set_text(add_btn, "Add Todo");
    vdom_set_event(add_btn, "onclick", "add-todo");
    vdom_append_child(div, add_btn);

    return div;
}

static VNode* render_todo_list(TodoState* state) {
    VNode* ul = vdom_create_element("ul");
    vdom_set_prop(ul, "class", "todo-list");

    int shown = 0;
    for (int i = 0; i < state->count; i++) {
        TodoItem* item = &state->items[i];
        bool show = false;
        if (strcmp(state->filter, "all") == 0) show = true;
        else if (strcmp(state->filter, "active") == 0 && !item->completed) show = true;
        else if (strcmp(state->filter, "completed") == 0 && item->completed) show = true;

        if (show) {
            VNode* li = render_todo_item(item->id, item->text, item->completed);
            vdom_append_child(ul, li);
            shown++;
        }
    }

    if (shown == 0) {
        VNode* empty = vdom_create_element("li");
        vdom_set_prop(empty, "class", "empty-list");
        VNode* em = vdom_create_element("em");
        vdom_set_text(em, "No todos to display");
        vdom_append_child(empty, em);
        vdom_append_child(ul, empty);
    }

    return ul;
}

static VNode* render_app_header(void) {
    VNode* header = vdom_create_element("header");
    vdom_set_prop(header, "class", "app-header");

    VNode* h1 = vdom_create_element("h1");
    vdom_set_text(h1, "Todo App");
    vdom_append_child(header, h1);

    VNode* subtitle = vdom_create_element("p");
    vdom_set_prop(subtitle, "class", "subtitle");
    vdom_set_text(subtitle, "Built with C99 Mini Frontend Framework");
    vdom_append_child(header, subtitle);

    return header;
}

static VNode* render_todo_app(Component* comp, VNode* props) {
    (void)props;
    (void)comp;

    static TodoState todo_state = {
        .count = 0, .next_id = 1, .filter = "all"
    };

    if (todo_state.count == 0) {
        todo_state.items[todo_state.count++] = (TodoItem){
            .id = todo_state.next_id++, .text = "Learn Virtual DOM concepts", .completed = true
        };
        todo_state.items[todo_state.count++] = (TodoItem){
            .id = todo_state.next_id++, .text = "Implement diff algorithm", .completed = true
        };
        todo_state.items[todo_state.count++] = (TodoItem){
            .id = todo_state.next_id++, .text = "Add hooks system", .completed = false
        };
        todo_state.items[todo_state.count++] = (TodoItem){
            .id = todo_state.next_id++, .text = "Build SPA router", .completed = false
        };
        todo_state.items[todo_state.count++] = (TodoItem){
            .id = todo_state.next_id++, .text = "Create reactive signals", .completed = false
        };
    }

    VNode* app = vdom_create_element("div");
    vdom_set_prop(app, "class", "todo-app");

    VNode* header = render_app_header();
    vdom_append_child(app, header);

    VNode* input_section = render_todo_input();
    vdom_append_child(app, input_section);

    VNode* filter_bar = render_filter_bar(todo_state.filter);
    vdom_append_child(app, filter_bar);

    VNode* list = render_todo_list(&todo_state);
    vdom_append_child(app, list);

    int completed = 0;
    for (int i = 0; i < todo_state.count; i++) {
        if (todo_state.items[i].completed) completed++;
    }
    VNode* stats = render_todo_stats(todo_state.count, completed);
    vdom_append_child(app, stats);

    return app;
}

static void run_todo_state_demo(void) {
    printf("\n--- Todo State Management Demo ---\n");

    TodoState state = { .count = 0, .next_id = 1, .filter = "all" };

    state.items[state.count++] = (TodoItem){
        .id = state.next_id++, .text = "Write C99 code", .completed = false
    };
    state.items[state.count++] = (TodoItem){
        .id = state.next_id++, .text = "Test the framework", .completed = false
    };

    printf("Created %d todos (next_id=%d)\n", state.count, state.next_id);

    state.items[0].completed = true;
    printf("Marked todo '%s' as completed\n", state.items[0].text);

    int active = 0, done = 0;
    for (int i = 0; i < state.count; i++) {
        if (state.items[i].completed) done++; else active++;
    }
    printf("Stats: %d active, %d completed\n", active, done);

    strcpy(state.filter, "active");
    printf("Filter set to: %s\n", state.filter);

    printf("Active todos:\n");
    for (int i = 0; i < state.count; i++) {
        if (!state.items[i].completed)
            printf("  [%d] %s\n", state.items[i].id, state.items[i].text);
    }
}

static void run_vdom_todo_rendering(void) {
    printf("\n--- VDOM Todo Rendering Demo ---\n");

    Component* todo_app = comp_create("TodoApp", render_todo_app);
    printf("Created component: %s\n", todo_app->name);

    VNode* rendered = comp_render(todo_app);
    printf("Rendered VDOM tree:\n");
    vdom_debug_print(rendered, 0);

    printf("\nCreating a patch scenario...\n");
    VNode* new_item = render_todo_item(99, "New dynamically added todo", false);
    printf("New item VNode:\n");
    vdom_debug_print(new_item, 0);

    vdom_free_recursive(new_item);
    vdom_free_recursive(rendered);
    comp_free(todo_app);
}

int main(void) {
    printf("========================================\n");
    printf("  Mini Frontend Framework - Todo Example\n");
    printf("========================================\n");

    printf("\n[Phase 1] State Management\n");
    run_todo_state_demo();

    printf("\n[Phase 2] VDOM Rendering\n");
    run_vdom_todo_rendering();

    printf("\n[Phase 3] Diff & Patch Simulation\n");

    VNode* old_list = vdom_create_element("ul");
    vdom_set_prop(old_list, "class", "todo-list");
    for (int i = 0; i < 3; i++) {
        VNode* li = render_todo_item(i + 1, (char*[]){"Task A", "Task B", "Task C"}[i], false);
        vdom_append_child(old_list, li);
    }

    printf("Old list:\n");
    vdom_debug_print(old_list, 0);

    VNode* new_list = vdom_create_element("ul");
    vdom_set_prop(new_list, "class", "todo-list");
    {
        VNode* li = render_todo_item(1, "Task A (updated)", true);
        vdom_set_key(li, "todo-1");
        vdom_append_child(new_list, li);
    }
    {
        VNode* li = render_todo_item(3, "Task C", false);
        vdom_set_key(li, "todo-3");
        vdom_append_child(new_list, li);
    }
    {
        VNode* li = render_todo_item(4, "Task D (new)", false);
        vdom_set_key(li, "todo-4");
        vdom_append_child(new_list, li);
    }

    printf("New list (removed B, updated A, added D):\n");
    vdom_debug_print(new_list, 0);

    printf("\nRunning diff between old and new lists...\n");
    vdom_diff(old_list, new_list, NULL);

    printf("\n[Phase 4] Cleanup\n");
    vdom_free_recursive(old_list);
    vdom_free_recursive(new_list);

    printf("\n=== Todo Example Completed ===\n");
    return 0;
}
