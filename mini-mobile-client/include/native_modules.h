#ifndef NATIVE_MODULES_H
#define NATIVE_MODULES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NM_MAX_MODULES      64
#define NM_MAX_CONSTANTS    32
#define NM_MAX_METHODS      48
#define NM_MAX_NAME_LEN     64
#define NM_MAX_PERM_NAME    128

typedef enum {
    NM_PERM_GRANTED  = 0,
    NM_PERM_DENIED   = 1,
    NM_PERM_UNKNOWN  = 2,
    NM_PERM_RESTRICT = 3
} nm_permission_state_t;

typedef enum {
    NM_PERM_CAMERA    = 0,
    NM_PERM_LOCATION  = 1,
    NM_PERM_STORAGE   = 2,
    NM_PERM_MIC       = 3,
    NM_PERM_CONTACTS  = 4,
    NM_PERM_NOTIF     = 5,
    NM_PERM_COUNT     = 6
} nm_permission_t;

typedef enum {
    NM_LIFECYCLE_CREATED  = 0,
    NM_LIFECYCLE_STARTING = 1,
    NM_LIFECYCLE_STARTED  = 2,
    NM_LIFECYCLE_STOPPING = 3,
    NM_LIFECYCLE_STOPPED  = 4,
    NM_LIFECYCLE_DESTROYED= 5
} nm_lifecycle_t;

typedef enum {
    NM_CONST_STRING = 0,
    NM_CONST_INT    = 1,
    NM_CONST_DOUBLE = 2,
    NM_CONST_BOOL   = 3
} nm_const_type_t;

typedef struct nm_constant {
    char           name[NM_MAX_NAME_LEN];
    nm_const_type_t type;
    union {
        char   str_val[NM_MAX_NAME_LEN];
        int64_t int_val;
        double  dbl_val;
        bool    bool_val;
    } value;
} nm_constant_t;

typedef struct nm_method_def {
    char name[NM_MAX_NAME_LEN];
    bool is_async;
    int  param_count;
} nm_method_def_t;

typedef int (*nm_method_impl)(const char *args, char *result, size_t result_len, void *ctx);
typedef void (*nm_async_impl)(const char *args, char *result, size_t result_len,
                               void *ctx, void (*done)(const char *, void *), void *cb_ctx);

typedef struct nm_module_def {
    char             name[NM_MAX_NAME_LEN];
    const char      *description;
    nm_constant_t    constants[NM_MAX_CONSTANTS];
    int              constant_count;
    nm_method_def_t  methods[NM_MAX_METHODS];
    int              method_count;
    nm_method_impl  *method_impls;
    nm_async_impl   *async_impls;
    nm_permission_t  permissions[4];
    int              perm_count;
    void            *instance_data;
    void            *(*create)(void);
    int              (*start)(void *instance);
    int              (*stop)(void *instance);
    void             (*destroy)(void *instance);
} nm_module_def_t;

typedef struct nm_module {
    nm_module_def_t  def;
    nm_lifecycle_t   state;
    void            *instance;
    bool             registered;
} nm_module_t;

typedef struct nm_registry {
    nm_module_t    modules[NM_MAX_MODULES];
    int            count;
    nm_permission_state_t perms[NM_PERM_COUNT];
} nm_registry_t;

int  nm_registry_init       (nm_registry_t *reg);
void nm_registry_destroy    (nm_registry_t *reg);

int  nm_register_module     (nm_registry_t *reg, const nm_module_def_t *def);

int  nm_start_module        (nm_registry_t *reg, const char *name);
int  nm_stop_module         (nm_registry_t *reg, const char *name);

int  nm_call_method         (nm_registry_t *reg, const char *module_name,
                              const char *method_name, const char *args,
                              char *result, size_t result_len);

int  nm_call_method_async   (nm_registry_t *reg, const char *module_name,
                              const char *method_name, const char *args,
                              char *result, size_t result_len,
                              void (*done)(const char *, void *), void *cb_ctx);

int  nm_get_constant         (nm_registry_t *reg, const char *module_name,
                              const char *const_name, nm_constant_t *out);

int  nm_get_constants        (nm_registry_t *reg, const char *module_name,
                              nm_constant_t *out, int max_count);

nm_permission_state_t nm_check_permission (nm_registry_t *reg, nm_permission_t perm);
void                  nm_set_permission   (nm_registry_t *reg, nm_permission_t perm,
                                           nm_permission_state_t state);

nm_module_t *nm_find_module    (nm_registry_t *reg, const char *name);
int          nm_module_count   (nm_registry_t *reg);

#ifdef __cplusplus
}
#endif

#endif
