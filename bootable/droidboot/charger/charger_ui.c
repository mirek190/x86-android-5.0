/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unistd.h>
#include <inttypes.h>
#include <cutils/klog.h>

#include "minui/minui.h"
#include "charger.h"
#include "charger_ui.h"

#define ARRAY_SIZE(x)           (sizeof(x)/sizeof(x[0]))

#define BATTERY_FULL_THRESH     95

#define LOGE(x...) do { KLOG_ERROR("charger", x); } while (0)
#define LOGI(x...) do { KLOG_INFO("charger", x); } while (0)
#define LOGV(x...) do { KLOG_DEBUG("charger", x); } while (0)

static struct frame batt_anim_frames[] = {
    {
        .name = "charger/battery_0",
        .disp_time = 750,
        .min_capacity = 0,
    },
    {
        .name = "charger/battery_0a",
        .disp_time = 750,
        .min_capacity = 20,
    },
    {
        .name = "charger/battery_1",
        .disp_time = 750,
        .min_capacity = 20,
    },
    {
        .name = "charger/battery_1a",
        .disp_time = 750,
        .min_capacity = 40,
    },
    {
        .name = "charger/battery_2",
        .disp_time = 750,
        .min_capacity = 40,
    },
    {
        .name = "charger/battery_3",
        .disp_time = 750,
        .min_capacity = 60,
    },
    {
        .name = "charger/battery_4",
        .disp_time = 750,
        .min_capacity = 80,
    },
    {
        .name = "charger/battery_5",
        .disp_time = 750,
        .min_capacity = BATTERY_FULL_THRESH,
    },
};

static struct animation battery_animation = {
    .frames = batt_anim_frames,
    .num_frames = ARRAY_SIZE(batt_anim_frames),
    .num_cycles = 3,
};

static struct animation *batt_anim = &battery_animation;
static gr_surface surf_unknown;

static void clear_screen(void)
{
    gr_color(0, 0, 0, 255);
    gr_fill(0, 0, gr_fb_width(), gr_fb_height());
};

void kick_animation(void)
{
    batt_anim->run = true;
}

void reset_animation(void)
{
    batt_anim->cur_cycle = 0;
    batt_anim->cur_frame = 0;
    batt_anim->run = false;
}

static int char_width;
static int char_height;

static int draw_text(const char *str, int x, int y)
{
    int str_len_px = gr_measure(str);

    if (x < 0)
        x = (gr_fb_width() - str_len_px) / 2;
    if (y < 0)
        y = (gr_fb_height() - char_height) / 2;
    gr_text(x, y, str, true);

    return y + char_height;
}

static void android_green(void)
{
    gr_color(0xa4, 0xc6, 0x39, 255);
}

/* returns the last y-offset of where the surface ends */
static int draw_surface_centered(gr_surface surface)
{
    int w;
    int h;
    int x;
    int y;

    w = gr_get_width(surface);
    h = gr_get_height(surface);
    x = (gr_fb_width() - w) / 2 ;
    y = (gr_fb_height() - h) / 2 ;

    LOGV("drawing surface %dx%d+%d+%d\n", w, h, x, y);
    gr_blit(surface, 0, 0, w, h, x, y);
    return y + h;
}

static void draw_unknown(void)
{
    int y;
    if (surf_unknown) {
        draw_surface_centered(surf_unknown);
    } else {
        android_green();
        y = draw_text("Charging!", -1, -1);
        draw_text("?\?/100", -1, y + 25);
    }
}

static void draw_battery(void)
{
    struct frame *frame = &batt_anim->frames[batt_anim->cur_frame];

    if (batt_anim->num_frames != 0) {
        draw_surface_centered(frame->surface);
        LOGV("drawing frame #%d name=%s min_cap=%d time=%d\n",
             batt_anim->cur_frame, frame->name, frame->min_capacity,
             frame->disp_time);
    }
}

static void redraw_screen(void)
{
    clear_screen();

    /* try to display *something* */
    if (batt_anim->capacity < 0 || batt_anim->num_frames == 0)
        draw_unknown();
    else
        draw_battery();
    gr_flip();
}

void update_screen_state(struct charger *charger, int64_t now)
{
    int disp_time;
    int batt_cap;

    if (!batt_anim->run || now < charger->next_screen_transition)
        return;

    /* animation is over, blank screen and leave */
    if (batt_anim->cur_cycle == batt_anim->num_cycles) {
        reset_animation();
        charger->next_screen_transition = -1;
        gr_fb_blank(true);
        LOGV("[%"PRId64"] animation done\n", now);
        if (charger->num_supplies_online != 0)
            set_screen_state(0);
        else {
            /* Stop at the correct-level, as animation could have
               ended at the next level */
            batt_cap = get_battery_capacity(charger);
            if (batt_cap < batt_anim->frames[batt_anim->anim_thresh].min_capacity)
                batt_anim->cur_frame = batt_anim->anim_thresh - 1;
            else
                batt_anim->cur_frame = batt_anim->anim_thresh;

            redraw_screen();
            reset_animation();
        }
        return;
    }

    disp_time = batt_anim->frames[batt_anim->cur_frame].disp_time;

    /* animation starting, set up the animation */
    if (batt_anim->cur_frame == 0) {
        LOGV("[%"PRId64"] animation starting\n", now);
        batt_cap = get_battery_capacity(charger);
        if (batt_cap >= 0 && batt_anim->num_frames != 0) {
            int i;

            /* find first frame given current capacity */
            for (i = 1; i < batt_anim->num_frames; i++) {
                if (batt_cap < batt_anim->frames[i].min_capacity)
                    break;
            }
            batt_anim->cur_frame = i - 1;
            /* Run animation only till the next segment */
            if (i == batt_anim->num_frames)
                batt_anim->anim_thresh = batt_anim->cur_frame;
            else
                batt_anim->anim_thresh = batt_anim->cur_frame + 1;

            /* show the first frame for twice as long */
            disp_time = batt_anim->frames[batt_anim->cur_frame].disp_time * 2;
        }

        batt_anim->capacity = batt_cap;
    }

    /* unblank the screen  on first cycle */
    if (batt_anim->cur_cycle == 0)
        gr_fb_blank(false);

    /* draw the new frame (@ cur_frame) */
    redraw_screen();

    /* if we don't have anim frames, we only have one image, so just bump
     * the cycle counter and exit
     */
    if (batt_anim->num_frames == 0 || batt_anim->capacity < 0) {
        LOGV("[%"PRId64"] animation missing or unknown battery status\n", now);
        charger->next_screen_transition = now + charger->batt_unknown_ms;
        batt_anim->cur_cycle++;
        return;
    }

    /* schedule next screen transition */
    charger->next_screen_transition = now + disp_time;


    /* advance frame cntr to the next valid frame if we are actually charging
     */
    if (charger->num_supplies_online != 0) {
        batt_anim->cur_frame++;

        /* if the frame is used for level-only, that is only show it when it's
         * the current level, skip it during the animation.
         */
        while (batt_anim->cur_frame < batt_anim->num_frames &&
               batt_anim->frames[batt_anim->cur_frame].level_only)
            batt_anim->cur_frame++;
        if (batt_anim->cur_frame > batt_anim->anim_thresh) {
            batt_anim->cur_cycle++;
            batt_anim->cur_frame = 0;

            /* don't reset the cycle counter, since we use that as a signal
             * in a test above to check if animation is over
             */
        }
    } else {
        /* Stop animating if we're not charging, but keep visible until
         * cycle count expires */
        batt_anim->cur_frame = 0;
        batt_anim->cur_cycle++;
    }
}

static void free_surfaces(void)
{
    for ( ; batt_anim->allocated_frames > 0; batt_anim->allocated_frames--)
        res_free_surface(batt_anim->frames[batt_anim->allocated_frames - 1].surface);
}

void init_font_size(void)
{
    gr_font_size(&char_width, &char_height);
}

void init_surfaces(void)
{
    int ret, i;

    ret = res_create_display_surface("charger/battery_fail", &surf_unknown);
    if (ret < 0) {
        LOGE("Cannot load image\n");
        surf_unknown = NULL;
    }

    for (i = 0; i < batt_anim->num_frames; i++) {
        struct frame *frame = &batt_anim->frames[i];

        ret = res_create_display_surface(frame->name, &frame->surface);
        if (ret < 0) {
            LOGE("Cannot load image %s\n", frame->name);
            free_surfaces();
            batt_anim->num_frames = 0;
            batt_anim->num_cycles = 1;
            break;
        }
        batt_anim->allocated_frames++;
    }
}

void clean_surfaces(enum charger_exit_state out_state, int min_charge)
{
    if (out_state == CHARGER_PROCEED && min_charge)
        gr_fb_blank(false);
    free_surfaces();
}

