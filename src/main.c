#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <bs_ini.h>

#include <vulkan.h>
#include <basilisk.h>

bs_Batch batch = { 0 };
bs_Renderer renderer = { 0 };

void tick() {
    bs_selectRenderer(&renderer);

    bs_selectBatch(&batch);
    bs_renderBatch(&batch, bs_batchRange(0, bs_batchSize(&batch)), BS_TRIANGLES);

    bs_commitRenderer(&renderer);
}

int main() {
    bs_ini(800, 600, "wnd");

    renderer = bs_renderer(bs_swapchainExtents().x, bs_swapchainExtents().y);
    bs_attach(&renderer, BS_COLOR_ATTACHMENT);
    bs_pushRenderer(&renderer);

    bs_VertexShader vs = bs_vertexShader("tri_vs.spv");
    bs_FragmentShader fs = bs_fragmentShader("tri_fs.spv");

    bs_Pipeline pipeline = bs_pipeline(&renderer, &vs, &fs);
    batch = bs_batch(&pipeline);
    bs_pushTriangle(&batch, bs_v3(0.0, -0.5, 0.0), bs_v3(0.5, 0.5, 0.0), bs_v3(-0.5, 0.5, 0.0), (bs_RGBA)BS_RED, NULL);
    bs_pushBatch(&batch);

    bs_run(tick);

    return 0;
}
