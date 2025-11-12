#pragma leco app
#pragma leco add_shader "poc.frag"
#pragma leco add_shader "poc.vert"

import casein;
import dotz;
import voo;
import vinyl;

struct app_stuff {
  voo::device_and_queue dq { "bait", casein::native_ptr };
  vee::render_pass rp = voo::single_att_render_pass(dq.physical_device(), dq.surface());

  vee::pipeline_layout pl = vee::create_pipeline_layout();
  vee::gr_pipeline gp = vee::create_graphics_pipeline({
    .pipeline_layout = *pl,
    .render_pass = *rp,
    .shaders {
      voo::shader("poc.vert.spv").pipeline_vert_stage(),
      voo::shader("poc.frag.spv").pipeline_frag_stage(),
    },
    .bindings {
      vee::vertex_input_bind(sizeof(dotz::vec2)),
    },
    .attributes {
      vee::vertex_attribute_vec2(0, 0),
    },
  });

  voo::one_quad quad { dq.physical_device() };
} * gas;

struct sized_stuff {
  voo::swapchain_and_stuff sw { gas->dq, *gas->rp };
} * gss;

static void on_start() {
  gas = new app_stuff {};
}
static void on_frame() {
  if (!gss) gss = new sized_stuff {};

  gss->sw.acquire_next_image();
  gss->sw.queue_one_time_submit(gas->dq.queue(), [] {
    auto cb = gss->sw.command_buffer();

    auto rp = gss->sw.cmd_render_pass({
      .command_buffer = gss->sw.command_buffer(),
    });
    vee::cmd_set_viewport(cb, gss->sw.extent());
    vee::cmd_set_scissor(cb, gss->sw.extent());
    vee::cmd_bind_gr_pipeline(cb, *gas->gp);
    gas->quad.run(cb, 0);
  });
  gss->sw.queue_present(gas->dq.queue());
}
static void on_resize() {
  if (gss) delete gss;
}
static void on_stop() {
  if (gss) delete gss;
  if (gas) delete gas;
}

const auto i = [] {
  using namespace vinyl;
  on(START, on_start);
  on(FRAME, on_frame);
  on(RESIZE, on_resize);
  on(STOP, on_stop);

  return 0;
}();
