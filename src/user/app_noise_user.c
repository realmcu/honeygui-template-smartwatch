#include "app_noise_user.h"
#include "time.h"

/**
 * 用户自定义实现
 * 此文件只生成一次，可自由修改
 */

// 在此添加自定义实现

/***
 * Template function
 * Distinguish development environments
 */
// void user_defined_func_called_by_event(void *obj, gui_event_t *e)
// {
//     GUI_UNUSED(obj);
//     GUI_UNUSED(e);
// #ifdef _HONEYGUI_SIMULATOR_
//     // TODO
// #else
//     // TODO
// #endif
// }

// void user_defined_func_called_by_msg(gui_obj_t *obj, const char *topic, void *data, uint16_t len)
// {
//     GUI_UNUSED(obj);
//     GUI_UNUSED(topic);
//     GUI_UNUSED(data);
//     GUI_UNUSED(len);
// #ifdef _HONEYGUI_SIMULATOR_
//     // TODO
// #else
//     // TODO
// #endif
// }


// 噪音级别相关变量
static int current_noise_level = 50;  // 当前噪音级别 (dB)
static int target_noise_level = 50;   // 目标噪音级别 (dB)
static gui_rounded_rect_t* noise_meters[15];   // 噪音柱数组

// 噪音级别颜色定义
typedef struct {
    int threshold;
    gui_color_t color;
} noise_color_t;

static const noise_color_t noise_colors[] = {
    {60, {.color.rgba = {0x71, 0xE1, 0x68, 0xFF}}},   // 绿色 (安全)
    {80, {.color.rgba = {0x00, 0xFF, 0xFF, 0xFF}}},   // 黄色 (注意)
    {100, {.color.rgba = {0x00, 0x66, 0xFF, 0xFF}}},  // 橙色 (警告)
    {120, {.color.rgba = {0x00, 0x00, 0xFF, 0xFF}}}   // 红色 (危险)
};

/**
 * 初始化噪音显示系统
 */
void app_noise_init(void)
{
    // 获取所有噪音柱的句柄
    noise_meters[0] = (gui_rounded_rect_t*)Nois_Level_Meter0;
    noise_meters[1] = (gui_rounded_rect_t*)Nois_Level_Meter1;
    noise_meters[2] = (gui_rounded_rect_t*)Nois_Level_Meter2;
    noise_meters[3] = (gui_rounded_rect_t*)Nois_Level_Meter3;
    noise_meters[4] = (gui_rounded_rect_t*)Nois_Level_Meter4;
    noise_meters[5] = (gui_rounded_rect_t*)Nois_Level_Meter5;
    noise_meters[6] = (gui_rounded_rect_t*)Nois_Level_Meter6;
    noise_meters[7] = (gui_rounded_rect_t*)Nois_Level_Meter7;
    noise_meters[8] = (gui_rounded_rect_t*)Nois_Level_Meter8;
    noise_meters[9] = (gui_rounded_rect_t*)Nois_Level_Meter9;
    noise_meters[10] = (gui_rounded_rect_t*)Nois_Level_Meter10;
    noise_meters[11] = (gui_rounded_rect_t*)Nois_Level_Meter11;
    noise_meters[12] = (gui_rounded_rect_t*)Nois_Level_Meter12;
    noise_meters[13] = (gui_rounded_rect_t*)Nois_Level_Meter13;
    noise_meters[14] = (gui_rounded_rect_t*)Nois_Level_Meter14;
    
    // 初始化随机数种子
    srand(time(NULL));
    
    // 立即显示初始状态（50dB）
    current_noise_level = 50;
    target_noise_level = 50;
    update_noise_meters(50);
    update_noise_status(50);
    
    // 启动噪音模拟定时器 (每100ms更新一次) - 使用 noise_meters[2] 避免与初始化定时器冲突
    gui_obj_create_timer((gui_obj_t*)noise_meters[2], 100, -1, noise_simulation_timer_cb);
    gui_obj_start_timer((gui_obj_t*)noise_meters[2]);
    
    // 启动显示更新定时器 (每50ms更新一次，实现平滑动画)
    gui_obj_create_timer((gui_obj_t*)noise_meters[3], 50, -1, noise_display_timer_cb);
    gui_obj_start_timer((gui_obj_t*)noise_meters[3]);
}

/**
 * 噪音模拟定时器回调 - 模拟真实噪音数据变化
 */
void noise_simulation_timer_cb(void *obj)
{
    GUI_UNUSED(obj);  // 避免未使用参数警告
    
    // 模拟噪音级别变化 (30-120 dB)
    static int noise_trend = 0;
    
    // 随机变化趋势
    if (rand() % 10 == 0) {
        noise_trend = (rand() % 3) - 1; // -1, 0, 1
    }
    
    // 应用趋势和随机波动
    int noise_change = noise_trend + (rand() % 7) - 3; // -3 到 +3 的随机变化
    target_noise_level += noise_change;
    
    // 限制范围
    if (target_noise_level < 30) target_noise_level = 30;
    if (target_noise_level > 120) target_noise_level = 120;
    
    // 更新显示的分贝数值
    static char noise_text[16];
    sprintf(noise_text, "%d dB", target_noise_level);
    gui_text_content_set((gui_text_t *)app_noise_data_text, noise_text, strlen(noise_text));
    
    // 更新状态文本和颜色
    update_noise_status(target_noise_level);
}

/**
 * 噪音显示定时器回调 - 实现平滑的柱状图动画
 */
void noise_display_timer_cb(void *obj)
{
    GUI_UNUSED(obj);  // 避免未使用参数警告
    
    // 平滑过渡到目标噪音级别
    if (current_noise_level < target_noise_level) {
        current_noise_level++;
    } else if (current_noise_level > target_noise_level) {
        current_noise_level--;
    }
    
    // 更新噪音柱显示
    update_noise_meters(current_noise_level);
}

/**
 * 更新噪音柱显示
 */
void update_noise_meters(int noise_db)
{
    // 计算应该显示的柱子数量 (30dB=0柱, 120dB=15柱)
    int active_meters = ((noise_db - 30) * 15) / 90;
    if (active_meters < 0) active_meters = 0;
    if (active_meters > 15) active_meters = 15;
    
    // 更新每个柱子的显示状态
    for (int i = 0; i < 15; i++) {
        if (noise_meters[i] != NULL) {
            if (i < active_meters) {
                // 显示柱子，根据级别设置颜色
                gui_color_t color = get_noise_color(noise_db);
                gui_rect_set_color(noise_meters[i], color);
                gui_rect_set_opacity(noise_meters[i], 255);
            } else {
                // 隐藏柱子
                gui_rect_set_opacity(noise_meters[i], 0);
            }
        }
    }
}

/**
 * 根据噪音级别获取颜色
 */
gui_color_t get_noise_color(int noise_db)
{
    for (size_t i = 0; i < sizeof(noise_colors)/sizeof(noise_colors[0]); i++) {
        if (noise_db <= noise_colors[i].threshold) {
            return noise_colors[i].color;
        }
    }
    // 默认红色
    gui_color_t red = {.color.rgba = {0x00, 0x00, 0xFF, 0xFF}};
    return red;
}

/**
 * 更新噪音状态文本
 */
void update_noise_status(int noise_db)
{
    const char* status_text;
    const char* icon_path;
    gui_color_t status_color;
    
    if (noise_db <= 70) {
        // OK 状态：≤70dB，绿色
        status_text = "OK";
        icon_path = "/app_noise/Noise_ok_icon.bin";
        status_color.color.rgba.r = 0x71;
        status_color.color.rgba.g = 0xE1;
        status_color.color.rgba.b = 0x68;
        status_color.color.rgba.a = 0xFF;
    } else {
        // LOUD 状态：>70dB，橙色
        status_text = "LOUD";
        icon_path = "/app_noise/Noise_warning_icon.bin";
        status_color.color.rgba.r = 0x00;
        status_color.color.rgba.g = 0xAD;
        status_color.color.rgba.b = 0xF4;
        status_color.color.rgba.a = 0xFF;
    }
    
    // 更新状态文本
    gui_text_content_set((gui_text_t *)app_noise_ok_text, (char*)status_text, strlen(status_text));
    gui_text_color_set((gui_text_t *)app_noise_ok_text, status_color);
    
    // 更新状态图标
    if (hg_image_1769156756841_h11r != NULL) {
        gui_img_t *icon_img = (gui_img_t *)hg_image_1769156756841_h11r;
        // 使用新的 API 替代废弃的 gui_img_set_attribute
        gui_img_set_src(icon_img, (void*)icon_path, IMG_SRC_FILESYS);
        gui_img_refresh_size(icon_img);
    }
}

/**
 * 设置噪音级别 (供外部调用)
 */
void app_noise_set_level(int noise_db)
{
    if (noise_db >= 30 && noise_db <= 120) {
        target_noise_level = noise_db;
    }
}

/**
 * 获取当前噪音级别
 */
int app_noise_get_level(void)
{
    return current_noise_level;
}