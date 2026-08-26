#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

static int screen_w = 0, screen_h = 0;

/*
 * 读取屏幕宽高
 */
void __am_gpu_init()
{
    uint32_t vgactl = inl(VGACTL_ADDR);
    screen_w = vgactl >> 16;
    screen_h = vgactl & 0xffff;
}

/*
 * 初始化 gpu 数据
 */
void __am_gpu_config(AM_GPU_CONFIG_T *cfg)
{
    *cfg = (AM_GPU_CONFIG_T){.present = true,
                             .has_accel = false,
                             .width = screen_w,
                             .height = screen_h,
                             .vmemsz = screen_w * screen_h * sizeof(uint32_t)};
}

/*
 * 将像素块写进帧缓冲
 */
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl)
{
    uint32_t *fb = (uint32_t *)FB_ADDR;
    for (int i = 0; i < ctl->h; i++)
    {
        for (int j = 0; j < ctl->w; j++)
        {
            fb[(ctl->y + i) * screen_w + (ctl->x + j)] = ((uint32_t *)ctl->pixels)[i * ctl->w + j];
        }
    }
    if (ctl->sync)
    {
        outl(SYNC_ADDR, 1); // 刷新屏幕
    }
}

void __am_gpu_status(AM_GPU_STATUS_T *status)
{
    status->ready = true;
}
