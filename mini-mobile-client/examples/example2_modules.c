#include "native_modules.h"
#include <stdio.h>
#include <string.h>

static void *camera_create(void)
{
    printf("[Camera] create\n");
    return (void *)(uintptr_t)1;
}

static int camera_start(void *instance)
{
    (void)instance;
    printf("[Camera] start\n");
    return 0;
}

static int camera_stop(void *instance)
{
    (void)instance;
    printf("[Camera] stop\n");
    return 0;
}

static void camera_destroy(void *instance)
{
    (void)instance;
    printf("[Camera] destroy\n");
}

static int camera_take_photo(const char *args, char *result, size_t result_len, void *ctx)
{
    (void)ctx;
    printf("[Camera::takePhoto] args=%s\n", args ? args : "(null)");
    snprintf(result, result_len, "{\"uri\":\"/photos/img_001.jpg\",\"width\":1920,\"height\":1080}");
    return 0;
}

static int camera_record_video(const char *args, char *result, size_t result_len, void *ctx)
{
    (void)ctx;
    printf("[Camera::recordVideo] args=%s\n", args ? args : "(null)");
    snprintf(result, result_len, "{\"uri\":\"/videos/vid_001.mp4\",\"duration\":12.5}");
    return 0;
}

static nm_method_impl camera_impls[] = { camera_take_photo, camera_record_video };

int main(void)
{
    nm_registry_t reg;
    nm_registry_init(&reg);

    nm_set_permission(&reg, NM_PERM_CAMERA, NM_PERM_GRANTED);
    printf("Camera permission granted\n");

    nm_module_def_t cam_def = {0};
    strcpy(cam_def.name, "Camera");
    cam_def.description = "Device camera module";

    cam_def.constants[0] = (nm_constant_t){
        .name = "Type",
        .type = NM_CONST_STRING,
        .value = { .str_val = "back" }
    };
    cam_def.constants[1] = (nm_constant_t){
        .name = "CameraType",
        .type = NM_CONST_STRING,
        .value = { .str_val = "back" }
    };
    cam_def.constant_count = 2;

    cam_def.methods[0] = (nm_method_def_t){ .name = "takePhoto", .is_async = true, .param_count = 1 };
    cam_def.methods[1] = (nm_method_def_t){ .name = "recordVideo", .is_async = true, .param_count = 2 };
    cam_def.method_count = 2;
    cam_def.method_impls = camera_impls;

    cam_def.permissions[0] = NM_PERM_CAMERA;
    cam_def.permissions[1] = NM_PERM_MIC;
    cam_def.perm_count      = 2;

    cam_def.create  = camera_create;
    cam_def.start   = camera_start;
    cam_def.stop    = camera_stop;
    cam_def.destroy = camera_destroy;

    nm_register_module(&reg, &cam_def);
    printf("Camera module registered\n");

    int rc = nm_start_module(&reg, "Camera");
    printf("Camera start: %d (state=%d)\n", rc, nm_find_module(&reg, "Camera")->state);

    char result[256];
    rc = nm_call_method(&reg, "Camera", "takePhoto",
                         "{\"quality\":0.9}", result, sizeof(result));
    printf("takePhoto result: %s (rc=%d)\n", result, rc);

    rc = nm_call_method(&reg, "Camera", "recordVideo",
                         "{\"quality\":\"1080p\",\"maxDuration\":30}", result, sizeof(result));
    printf("recordVideo result: %s (rc=%d)\n", result, rc);

    nm_constant_t c;
    rc = nm_get_constant(&reg, "Camera", "Type", &c);
    printf("Constant Camera.Type: %s (rc=%d)\n", c.value.str_val, rc);

    nm_constant_t all_consts[8];
    int nc = nm_get_constants(&reg, "Camera", all_consts, 8);
    printf("All constants: %d\n", nc);
    for (int i = 0; i < nc; i++) {
        printf("  %s\n", all_consts[i].name);
    }

    printf("Module count: %d\n", nm_module_count(&reg));

    rc = nm_stop_module(&reg, "Camera");
    printf("Camera stop: %d\n", rc);

    nm_registry_destroy(&reg);
    printf("Registry destroyed\n");

    return 0;
}
