#include "native_modules.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int nm_registry_init(nm_registry_t *reg)
{
    if (!reg) return -1;
    memset(reg, 0, sizeof(*reg));

    for (int i = 0; i < NM_PERM_COUNT; i++) {
        reg->perms[i] = NM_PERM_UNKNOWN;
    }

    return 0;
}

void nm_registry_destroy(nm_registry_t *reg)
{
    if (!reg) return;

    for (int i = 0; i < reg->count; i++) {
        nm_module_t *mod = &reg->modules[i];
        if (mod->state >= NM_LIFECYCLE_STARTED) {
            if (mod->def.stop) mod->def.stop(mod->instance);
        }
        if (mod->def.destroy) mod->def.destroy(mod->instance);
        mod->state = NM_LIFECYCLE_DESTROYED;
    }

    memset(reg, 0, sizeof(*reg));
}

int nm_register_module(nm_registry_t *reg, const nm_module_def_t *def)
{
    if (!reg || !def || !def->name[0]) return -1;
    if (reg->count >= NM_MAX_MODULES) return -1;

    for (int i = 0; i < reg->count; i++) {
        if (strcmp(reg->modules[i].def.name, def->name) == 0) {
            reg->modules[i].def = *def;
            return 0;
        }
    }

    nm_module_t *mod = &reg->modules[reg->count++];
    memcpy(&mod->def, def, sizeof(*def));
    mod->state      = NM_LIFECYCLE_CREATED;
    mod->instance   = NULL;
    mod->registered = true;

    return 0;
}

static nm_module_t *nm_find_internal(nm_registry_t *reg, const char *name)
{
    for (int i = 0; i < reg->count; i++) {
        if (strcmp(reg->modules[i].def.name, name) == 0) {
            return &reg->modules[i];
        }
    }
    return NULL;
}

int nm_start_module(nm_registry_t *reg, const char *name)
{
    nm_module_t *mod = nm_find_internal(reg, name);
    if (!mod) return -1;
    if (mod->state >= NM_LIFECYCLE_STARTING) return 0;

    for (int i = 0; i < mod->def.perm_count; i++) {
        if (reg->perms[mod->def.permissions[i]] != NM_PERM_GRANTED) {
            return -2;
        }
    }

    mod->state = NM_LIFECYCLE_STARTING;

    if (mod->def.create) {
        mod->instance = mod->def.create();
    }

    if (mod->def.start) {
        if (mod->def.start(mod->instance) != 0) {
            if (mod->def.destroy) mod->def.destroy(mod->instance);
            mod->instance = NULL;
            mod->state = NM_LIFECYCLE_CREATED;
            return -3;
        }
    }

    mod->state = NM_LIFECYCLE_STARTED;
    return 0;
}

int nm_stop_module(nm_registry_t *reg, const char *name)
{
    nm_module_t *mod = nm_find_internal(reg, name);
    if (!mod) return -1;
    if (mod->state < NM_LIFECYCLE_STARTED) return 0;

    mod->state = NM_LIFECYCLE_STOPPING;

    if (mod->def.stop) {
        mod->def.stop(mod->instance);
    }

    mod->state = NM_LIFECYCLE_STOPPED;
    return 0;
}

static int nm_find_method(nm_module_t *mod, const char *method_name)
{
    for (int i = 0; i < mod->def.method_count; i++) {
        if (strcmp(mod->def.methods[i].name, method_name) == 0) {
            return i;
        }
    }
    return -1;
}

int nm_call_method(nm_registry_t *reg, const char *module_name,
                    const char *method_name, const char *args,
                    char *result, size_t result_len)
{
    nm_module_t *mod = nm_find_internal(reg, module_name);
    if (!mod) return -1;
    if (mod->state != NM_LIFECYCLE_STARTED) return -2;

    int mi = nm_find_method(mod, method_name);
    if (mi < 0) return -3;

    if (mod->def.method_impls && mod->def.method_impls[mi]) {
        return mod->def.method_impls[mi](args, result, result_len, mod->instance);
    }

    return -4;
}

int nm_call_method_async(nm_registry_t *reg, const char *module_name,
                          const char *method_name, const char *args,
                          char *result, size_t result_len,
                          void (*done)(const char *, void *), void *cb_ctx)
{
    nm_module_t *mod = nm_find_internal(reg, module_name);
    if (!mod) return -1;
    if (mod->state != NM_LIFECYCLE_STARTED) return -2;

    int mi = nm_find_method(mod, method_name);
    if (mi < 0) return -3;

    if (mod->def.async_impls && mod->def.async_impls[mi]) {
        mod->def.async_impls[mi](args, result, result_len, mod->instance, done, cb_ctx);
        return 0;
    }

    return -4;
}

int nm_get_constant(nm_registry_t *reg, const char *module_name,
                     const char *const_name, nm_constant_t *out)
{
    nm_module_t *mod = nm_find_internal(reg, module_name);
    if (!mod || !out) return -1;

    for (int i = 0; i < mod->def.constant_count; i++) {
        if (strcmp(mod->def.constants[i].name, const_name) == 0) {
            memcpy(out, &mod->def.constants[i], sizeof(*out));
            return 0;
        }
    }

    return -2;
}

int nm_get_constants(nm_registry_t *reg, const char *module_name,
                      nm_constant_t *out, int max_count)
{
    nm_module_t *mod = nm_find_internal(reg, module_name);
    if (!mod || !out) return -1;

    int n = mod->def.constant_count;
    if (n > max_count) n = max_count;
    memcpy(out, mod->def.constants, n * sizeof(nm_constant_t));
    return n;
}

nm_permission_state_t nm_check_permission(nm_registry_t *reg, nm_permission_t perm)
{
    if (!reg || perm >= NM_PERM_COUNT) return NM_PERM_DENIED;
    return reg->perms[perm];
}

void nm_set_permission(nm_registry_t *reg, nm_permission_t perm,
                        nm_permission_state_t state)
{
    if (!reg || perm >= NM_PERM_COUNT) return;
    reg->perms[perm] = state;
}

nm_module_t *nm_find_module(nm_registry_t *reg, const char *name)
{
    return nm_find_internal(reg, name);
}

int nm_module_count(nm_registry_t *reg)
{
    return reg ? reg->count : 0;
}
