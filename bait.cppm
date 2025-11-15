#pragma leco add_shader "bait.vert"
#pragma leco add_shader "bait.frag"
#pragma leco app

export module bait;
import :descriptors;
import :offscreen;
import :pipeline;
import :surface;
import casein;
import sitime;
import vee;
import vinyl;

static volatile bool g_shot;

class thread {
  sitime::stopwatch watch {};
  voo::device_and_queue dq { "bait", casein::native_ptr };
  voo::one_quad q_buf { dq.physical_device() };
  offscreen_framebuffer osfb { dq.physical_device() };
  surface_framebuffer sfb { dq };
  desc_set ds { &dq };
  base_pipeline_layout bpl { ds.layout() };

public:
  thread() {
    osfb.set_pipeline(bpl.create_graphics_pipeline(osfb.render_pass()));
    sfb.set_pipeline(bpl.create_graphics_pipeline(*sfb.render_pass()));
  }
  void frame() {
    auto cb = sfb.cb();
    sfb.wait_reset_and_acquire();

    const auto render = [&](auto &fb) {
      vee::cmd_bind_descriptor_set(cb, *bpl, 0, *ds);
      vee::cmd_push_vert_frag_constants(cb, *bpl, &fb.push_constants());

      q_buf.run(cb, 0);
      vee::cmd_end_render_pass(cb);
    };

    // TODO: add a mutex or an atomic
    bool shoot = g_shot;
    g_shot = false;

    float time = 0.001 * watch.millis();
    osfb.push_constants().time = time;
    sfb.push_constants().time = time;

    // Build command buffer
    vee::begin_cmd_buf_one_time_submit(cb);
    {
      sfb.cmd_begin_render_pass(cb);
      render(sfb);

      if (shoot) {
        osfb.cmd_begin_render_pass(cb);
        render(osfb);
      }
    }
    if (shoot) osfb.cmd_copy_to_buffer(cb);
    vee::end_cmd_buf(cb);

    // Submit and present
    sfb.submit_and_present(dq.queue());

    // Pull data from buffer
    if (shoot) osfb.write_buffer_to_file();
  }
};

const int i = [] {
  static thread * t;

  using namespace vinyl;
  on(START, [] { t = new thread{}; });
  on(RESIZE, [] {});
  on(FRAME, [] { t->frame(); });
  on(STOP, [] { delete t; });

  using namespace casein;
  handle(MOUSE_DOWN, [] { g_shot = true; });

  return 0;
}();
