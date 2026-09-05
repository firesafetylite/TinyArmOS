/* Optional macOS test adapter for UTM's QEMU 10 exported lifecycle API.
 * Runs a separate emulator process; does not read UTM VM configurations.
 * Build: cc tools/utm_qemu_runner.c \
 *   -Wl,-rpath,/Applications/UTM.app/Contents/Frameworks -o /tmp/tinygpt-qemu
 * This is an internal QEMU API and may need updating for other UTM versions.
 * Use only with the TCG smoke tests, not a running VM's disk.
 */
#include <dlfcn.h>
#include <stdio.h>
int main(int argc, char **argv) {
    void *library = dlopen("/Applications/UTM.app/Contents/Frameworks/"
        "qemu-aarch64-softmmu.framework/qemu-aarch64-softmmu", RTLD_NOW | RTLD_GLOBAL);
    if (!library) { fprintf(stderr, "%s\n", dlerror()); return 1; }
    void (*initialize)(int, char **) = dlsym(library, "qemu_init");
    void (*main_loop)(void) = dlsym(library, "qemu_main_loop");
    void (*cleanup)(void) = dlsym(library, "qemu_cleanup");
    if (!initialize || !main_loop || !cleanup) {
        fprintf(stderr, "UTM's QEMU lifecycle exports are unavailable\n"); return 1;
    }
    initialize(argc, argv);
    main_loop();
    cleanup();
    return 0;
}
