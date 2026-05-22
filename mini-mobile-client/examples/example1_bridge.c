#include "react_native_bridge.h"
#include <stdio.h>
#include <string.h>

static void camera_handler(const char *args, rn_callback_fn resolve, rn_callback_fn reject)
{
    (void)reject;
    printf("[Camera] called with args: %s\n", args ? args : "(null)");
    if (resolve) resolve("{\"status\":\"ok\",\"photo\":\"base64data...\"}", NULL);
}

static void location_handler(const char *args, rn_callback_fn resolve, rn_callback_fn reject)
{
    (void)reject;
    printf("[Location] called with args: %s\n", args ? args : "(null)");
    if (resolve) resolve("{\"lat\":37.7749,\"lng\":-122.4194}", NULL);
}

int main(void)
{
    rn_bridge_t bridge;
    rn_status_t st = rn_bridge_init(&bridge);
    printf("Bridge init: %d\n", st);

    rn_bridge_register_module(&bridge, "Camera", camera_handler);
    rn_bridge_register_module(&bridge, "Location", location_handler);

    char *enc1 = rn_bridge_encode_call("Camera", "takePhoto",
                                        "{\"quality\":0.8,\"base64\":true}");
    printf("Encoded call: %s\n", enc1);

    rn_bridge_send_message(&bridge, RN_THREAD_UI, "Camera", "takePhoto",
                           "{\"quality\":0.8,\"base64\":true}");
    rn_bridge_send_message(&bridge, RN_THREAD_UI, "Location", "getCurrentPosition",
                           "{\"enableHighAccuracy\":true}");

    printf("Pending messages: %d\n", rn_bridge_pending_count(&bridge));

    st = rn_bridge_flush(&bridge, RN_THREAD_UI);
    printf("Flushed UI thread messages: %d\n", st);
    printf("Pending after flush: %d\n", rn_bridge_pending_count(&bridge));

    char module[64], method[64], args[512];
    st = rn_bridge_decode_call(enc1, module, method, args, sizeof(args));
    printf("Decoded: module=%s method=%s args=%s (status=%d)\n", module, method, args, st);

    int32_t cb_id = rn_bridge_register_callback(&bridge, NULL, NULL, NULL, 5000);
    printf("Registered callback ID: %d\n", cb_id);

    rn_bridge_send_callback(&bridge, cb_id, "{\"done\":true}");
    printf("Pending after callback: %d\n", rn_bridge_pending_count(&bridge));

    rn_bridge_flush(&bridge, RN_THREAD_JS);
    printf("Pending after JS flush: %d\n", rn_bridge_pending_count(&bridge));

    rn_bridge_set_batching(&bridge, false);
    printf("Batching disabled\n");

    free(enc1);
    rn_bridge_destroy(&bridge);
    printf("Bridge destroyed\n");

    return 0;
}
