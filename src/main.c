#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <bs_ini.h>

#include <vulkan.h>

void draw() {
    //vkWaitForFences(device, 1, &render_fence, true, 1000000000);
    //vkResetFences(device, 1, &render_fence);

    //vkAcquireNextImageKHR(device, )
}

void tick() {
    draw();
}

int main() {
    bs_ini(800, 600, "wnd");

    bs_run(tick);

    return 0;
}
