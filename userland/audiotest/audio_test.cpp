/* PipeWire */
/* SPDX-FileCopyrightText: Copyright © 2018 Wim Taymans */
/* SPDX-License-Identifier: MIT */

/*
 [title]
 Audio source using \ref pw_stream "pw_stream".
 [title]
 */

#include <di/types/integers.h>
#include <dius/print.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

#define M_PI_M2 (M_PI + M_PI)

#define DEFAULT_RATE     44100
#define DEFAULT_CHANNELS 2
#define DEFAULT_VOLUME   0.5f

struct data {
    struct pw_main_loop* loop;
    struct pw_stream* stream;

    double accumulator;
};

static void fill_f32(struct data* d, void* dest, int n_frames) {
    auto* dst = (float*) dest;
    float val;
    int i;
    int c;
    static int f = 220;
    for (i = 0; i < n_frames; i++) {
        d->accumulator += M_PI_M2 * f / DEFAULT_RATE;
        if (d->accumulator >= M_PI_M2) {
            d->accumulator -= M_PI_M2;
        }

        val = sinf(d->accumulator) * DEFAULT_VOLUME;
        for (c = 0; c < DEFAULT_CHANNELS; c++) {
            *dst++ = val;
        }
    }
    f++;
    if (f == 880) {
        f = 220;
    }
}

/* our data processing function is in general:
 *
 *  struct pw_buffer *b;
 *  b = pw_stream_dequeue_buffer(stream);
 *
 *  .. generate stuff in the buffer ...
 *
 *  pw_stream_queue_buffer(stream, b);
 */
static void on_process(void* userdata) {
    auto* data = (struct data*) userdata;
    struct pw_buffer* b;
    struct spa_buffer* buf;
    int n_frames;
    int stride;
    uint8_t* p;

    if ((b = pw_stream_dequeue_buffer(data->stream)) == nullptr) {
        dius::eprintln("out of buffers"_sv);
        return;
    }

    buf = b->buffer;
    if ((p = (u8*) buf->datas[0].data) == nullptr) {
        return;
    }

    stride = sizeof(float) * DEFAULT_CHANNELS;
    n_frames = di::min(i32(b->requested), i32(buf->datas[0].maxsize / stride));

    fill_f32(data, p, n_frames);

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = n_frames * stride;

    pw_stream_queue_buffer(data->stream, b);
}

static const struct pw_stream_events stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

static void do_quit(void* userdata, int) {
    auto* data = (struct data*) userdata;
    pw_main_loop_quit(data->loop);
}

int main(int argc, char* argv[]) {
    struct data data = {};
    spa_pod const* params[1];
    uint8_t buffer[1024];
    struct pw_properties* props;
    struct spa_pod_builder b = {};
    b.data = buffer;
    b.size = sizeof(buffer);

    pw_init(&argc, &argv);

    /* make a main loop. If you already have another main loop, you can add
     * the fd of this pipewire mainloop to it. */
    data.loop = pw_main_loop_new(nullptr);

    __extension__ pw_loop_add_signal(pw_main_loop_get_loop(data.loop), (u32) SIGINT, do_quit, &data);
    __extension__ pw_loop_add_signal(pw_main_loop_get_loop(data.loop), (u32) SIGTERM, do_quit, &data);

    /* Create a simple stream, the simple stream manages the core and remote
     * objects for you if you don't need to deal with them.
     *
     * If you plan to autoconnect your stream, you need to provide at least
     * media, category and role properties.
     *
     * Pass your events and a user_data pointer as the last arguments. This
     * will inform you about the stream state. The most important event
     * you need to listen to is the process event where you need to produce
     * the data.
     */
    props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Playback", PW_KEY_MEDIA_ROLE, "Music",
                              NULL);
    if (argc > 1) {
        /* Set stream target if given on command line */
        pw_properties_set(props, PW_KEY_TARGET_OBJECT, argv[1]);
    }
    data.stream = pw_stream_new_simple(pw_main_loop_get_loop(data.loop), "audio-src", props, &stream_events, &data);

    /* Make one parameter with the supported formats. The SPA_PARAM_EnumFormat
     * id means that this is a format enumeration (of 1 value). */
    spa_audio_info_raw audio_init = {};
    audio_init.format = SPA_AUDIO_FORMAT_F32;
    audio_init.rate = DEFAULT_RATE;
    audio_init.channels = DEFAULT_CHANNELS;
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &audio_init);

    /* Now connect this stream. We ask that our process function is
     * called in a realtime thread. */
    pw_stream_connect(
        data.stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
        pw_stream_flags(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS), params,
        1);

    /* and wait while we let things run */
    pw_main_loop_run(data.loop);

    pw_stream_destroy(data.stream);
    pw_main_loop_destroy(data.loop);
    pw_deinit();

    return 0;
}
