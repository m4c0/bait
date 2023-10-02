module;
#define offsetof(t, d) __builtin_offsetof(t, d)

export module bait;
import :descriptors;
import :offscreen;
import :pipeline;
import vee;

struct quad {
  point p[6]{
      {0.0, 0.0}, {1.0, 1.0}, {1.0, 0.0},

      {1.0, 1.0}, {0.0, 0.0}, {0.0, 1.0},
  };
};

extern "C" int main() {
  // Instance
  vee::instance i = vee::create_instance("bait");
  vee::debug_utils_messenger dbg = vee::create_debug_utils_messenger();
  auto [pd, qf] = vee::find_physical_device_with_universal_queue(nullptr);

  // Device
  vee::device d = vee::create_single_queue_device(pd, qf);
  vee::queue q = vee::get_queue_for_family(qf);

  // Inputs (vertices + instance)
  vee::buffer q_buf = vee::create_vertex_buffer(sizeof(quad));
  vee::device_memory q_mem = vee::create_host_buffer_memory(pd, sizeof(quad));
  vee::bind_buffer_memory(*q_buf, *q_mem, 0);
  {
    vee::mapmem mem{*q_mem};
    *static_cast<quad *>(*mem) = {};
  }

  desc_set ds{pd};
  base_pipeline_layout bpl{ds.layout()};

  offscreen_framebuffer osfb{pd};
  osfb.set_pipeline(bpl.create_graphics_pipeline(osfb.render_pass()));

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

#pragma leco add_shader "bait.vert"
#pragma leco add_shader "bait.frag"
#pragma leco tool
