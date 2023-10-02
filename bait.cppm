export module bait;
import :descriptors;
import :offscreen;
import :pipeline;
import :quad;
import :surface;
import casein;
import sith;
import sitime;
import vee;

class thread : public sith::thread {
  casein::native_handle_t m_nptr;
  volatile bool m_resized;

public:
  void start(casein::native_handle_t n) {
    m_nptr = n;
    sith::thread::start();
  }
  void resize() { m_resized = true; }

  void run() override;
};

void thread::run() {
  sitime::stopwatch watch{};

  // Instance
  vee::instance i = vee::create_instance("bait");
  vee::debug_utils_messenger dbg = vee::create_debug_utils_messenger();
  vee::surface s = vee::create_surface(m_nptr);
  auto [pd, qf] = vee::find_physical_device_with_universal_queue(*s);

  // Device
  vee::device d = vee::create_single_queue_device(pd, qf);
  vee::queue q = vee::get_queue_for_family(qf);

  quad_buf q_buf{pd};
  desc_set ds{pd};
  base_pipeline_layout bpl{ds.layout()};

  offscreen_framebuffer osfb{pd};
  osfb.set_pipeline(bpl.create_graphics_pipeline(osfb.render_pass()));

  surface_framebuffer sfb{pd, s};
  sfb.set_pipeline(bpl.create_graphics_pipeline(sfb.render_pass()));

  // Command pool + buffer
  vee::command_pool cp = vee::create_command_pool(qf);
  vee::command_buffer cb = vee::allocate_primary_command_buffer(*cp);

  // Build command buffer
  {
    upc pc{
        .aspect = osfb.aspect(),
        .time = 0.0,
    };

    vee::begin_cmd_buf_one_time_submit(cb);
    {
      ds.cmd_prepare_images(cb);

      osfb.cmd_begin_render_pass(cb);
      vee::cmd_bind_descriptor_set(cb, *bpl, 0, *ds);
      vee::cmd_push_vert_frag_constants(cb, *bpl, &pc);

      vee::cmd_bind_vertex_buffers(cb, 0, *q_buf);
      vee::cmd_draw(cb, 6);
      vee::cmd_end_render_pass(cb);
    }
    osfb.cmd_copy_to_buffer(cb);
    vee::end_cmd_buf(cb);
  }

  // Submit
  vee::queue_submit({
      .queue = q,
      .command_buffer = cb,
  });

  // Pull data from buffer
  osfb.write_buffer_to_file();
}

extern "C" void casein_handle(const casein::event &e) {
  static thread t{};

  static constexpr auto map = [] {
    casein::event_map res{};
    res[casein::CREATE_WINDOW] = [](const casein::event &e) {
      t.start(*e.as<casein::events::create_window>());
    };
    res[casein::RESIZE_WINDOW] = [](auto) { t.resize(); };
    res[casein::QUIT] = [](auto) { t.stop(); };
    return res;
  }();

  map.handle(e);
}

#pragma leco add_shader "bait.vert"
#pragma leco add_shader "bait.frag"
#pragma leco tool
