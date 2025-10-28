#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"

#include "UICore.h"

#include "debug.h"
#include "keyboard_gii39.h"

#define DISP_HOR_RES 256
#define VBUFFER_LINE 127

uint8_t g_ShiftStatus = 0;
uint8_t g_AlphaStatus = 0;

static bool OS_UISuspend = false;

static TaskHandle_t lvgl_svc_task;
static TaskHandle_t lvgl_tick_task;

static lv_disp_draw_buf_t draw_buf_dsc_1;
static lv_color_t disp_buf_1[DISP_HOR_RES * VBUFFER_LINE];
static lv_disp_drv_t disp_1_drv;
static lv_indev_drv_t indev_drv;
static lv_indev_t *indev_keypad;

static lv_group_t *group;

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    ll_disp_put_area((uint8_t *)color_p, area->x1, area->y1, area->x2, area->y2);
    lv_disp_flush_ready(disp_drv);
}

void lvgl_tick() {
    for (;;) {
        lv_tick_inc(51);
        vTaskDelay(pdMS_TO_TICKS(52));
    }
}

void lvgl_svc() {
    vTaskDelay(pdMS_TO_TICKS(400));
    for (;;) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(62));
    }
}

lv_indev_t *SystemGetInKeypad()
{
    return indev_keypad;
}

void SystemUIInit() {
    lv_init();

    lv_disp_draw_buf_init(&draw_buf_dsc_1, disp_buf_1, NULL, DISP_HOR_RES * VBUFFER_LINE);
    lv_disp_drv_init(&disp_1_drv);
    disp_1_drv.hor_res = 256;
    disp_1_drv.ver_res = 127;
    disp_1_drv.flush_cb = disp_flush;
    disp_1_drv.draw_buf = &draw_buf_dsc_1;
    lv_disp_drv_register(&disp_1_drv);

    /*Register a button input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_keypad = lv_indev_drv_register(&indev_drv);
    
    xTaskCreate(lvgl_svc, "lvgl svc", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, &lvgl_svc_task);
    xTaskCreate(lvgl_tick, "lvgl tick", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, &lvgl_tick_task);

    vTaskDelay(pdMS_TO_TICKS(100));
    
    group = lv_group_create();
    lv_group_set_default(group);

    lv_indev_t *cur_drv = NULL;
    for (;;) {
        cur_drv = lv_indev_get_next(cur_drv);
        if (!cur_drv) {
            break;
        }

        if (cur_drv->driver->type == LV_INDEV_TYPE_KEYPAD) {
            lv_indev_set_group(cur_drv, group);
        }
    }

    lv_group_set_refocus_policy(group, LV_GROUP_REFOCUS_POLICY_NEXT);
}

void SystemUIEditing(bool edit) {
    lv_group_set_editing(group, edit);
}

void SystemUISetBusy(bool enable)
{
    // 简化的忙碌状态设置
    // 具体实现可以根据需要添加
}

void SystemUISuspend() {
    OS_UISuspend = true;
    vTaskSuspend(lvgl_svc_task);
    vTaskSuspend(lvgl_tick_task);
}

void SystemUIResume() {
    vTaskResume(lvgl_svc_task);
    vTaskResume(lvgl_tick_task);
    OS_UISuspend = false;
}

void UI_OOM() {
    // 内存不足处理函数
    // 可以添加内存不足时的处理逻辑
    printf("UI Out of Memory!\n");
}