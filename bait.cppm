module;
#define offsetof(t, d) __builtin_offsetof(t, d)

export module bait;
import silog;
import stubby;
import vee;

static constexpr const auto width = 1280;
static constexpr const auto height = 768;
static constexpr const auto filename = "out/test.jpg";

struct point {
  float x;
  float y;
};
struct quad {
  point p[6]{
      {0.0, 0.0}, {1.0, 1.0}, {1.0, 0.0},

      {1.0, 1.0}, {0.0, 0.0}, {0.0, 1.0},
  };
};

struct colour {
  float r;
  float g;
  float b;
  float a;
};
struct area {
  float x;
  float y;
  float w;
  float h;
};
struct inst {
  colour from;
  colour to;
  area rect;
};
struct all {
  quad q{};
  inst i[2]{
      inst{
          .from = {1.0, 1.0, 1.0, 1.0},
          .to = {0, 0, 0, 1},
          .rect = {-1.0, -1.0, 1.0, 2.0},
      },
      inst{
          .from = {0.0, 1.0, 1.0, 1.0},
          .to = {1, 0, 0, 1},
          .rect = {0.0, -1.0, 1.0, 2.0},
      },
  };
};

class bound_quad {
  vee::physical_device m_pd;

  vee::buffer m_quad = vee::create_vertex_buffer(sizeof(all::q));
  vee::buffer m_inst = vee::create_vertex_buffer(sizeof(all::i));

  vee::device_memory m_mem = vee::create_host_buffer_memory(m_pd, sizeof(all));

public:
  bound_quad(vee::physical_device pd) : m_pd{pd} {
    vee::bind_buffer_memory(*m_quad, *m_mem, 0);
    vee::bind_buffer_memory(*m_inst, *m_mem, sizeof(quad));

    vee::mapmem mem{*m_mem};
    *static_cast<all *>(*mem) = {};
  }

  void run(vee::command_buffer cb) {
    vee::cmd_bind_vertex_buffers(cb, 0, *m_quad);
    vee::cmd_bind_vertex_buffers(cb, 1, *m_inst);
    vee::cmd_draw(cb, 6, sizeof(all::i) / sizeof(inst));
  }
};

struct upc {
  float aspect;
};
class pipeline {
  const vee::render_pass &m_rp;

  vee::shader_module m_vert =
      vee::create_shader_module_from_resource("bait.vert.spv");
  vee::shader_module m_frag =
      vee::create_shader_module_from_resource("bait.frag.spv");
  vee::pipeline_layout m_pl =
      vee::create_pipeline_layout({vee::vert_frag_push_constant_range<upc>()});
  vee::gr_pipeline m_gp = vee::create_graphics_pipeline(
      *m_pl, *m_rp,
      {
          vee::pipeline_vert_stage(*m_vert, "main"),
          vee::pipeline_frag_stage(*m_frag, "main"),
      },
      {
          vee::vertex_input_bind(sizeof(point)),
          vee::vertex_input_bind_per_instance(sizeof(inst)),
      },
      {
          vee::vertex_attribute_vec2(0, 0),
          vee::vertex_attribute_vec4(1, offsetof(inst, from)),
          vee::vertex_attribute_vec4(1, offsetof(inst, to)),
          vee::vertex_attribute_vec4(1, offsetof(inst, rect)),
      });

  upc m_pc;

public:
  pipeline(const vee::render_pass &rp) : m_rp{rp} {}

  void run(vee::command_buffer cb) {
    m_pc.aspect = static_cast<float>(width) / static_cast<float>(height);

    vee::cmd_bind_gr_pipeline(cb, *m_gp);
    vee::cmd_push_vert_frag_constants(cb, *m_pl, &m_pc);
  }
};

class program {
  bound_quad m_t;
  pipeline m_p;

public:
  program(const vee::physical_device pd, const vee::render_pass &rp)
      : m_t{pd}, m_p{rp} {}

  void run(vee::command_buffer cb) {
    m_p.run(cb);
    m_t.run(cb);
  }
};

extern "C" int main() {
  vee::instance i = vee::create_instance("bait");
  auto [pd, qf] = vee::find_physical_device_with_universal_queue(nullptr);

  vee::device d = vee::create_single_queue_device(pd, qf);
  vee::queue q = vee::get_queue_for_family(qf);

  vee::render_pass rp = vee::create_render_pass(pd, nullptr);

  vee::image t_img = vee::create_renderable_image({width, height});
  vee::device_memory t_mem = vee::create_local_image_memory(pd, *t_img);
  vee::bind_image_memory(*t_img, *t_mem);
  vee::image_view t_iv = vee::create_srgba_image_view(*t_img);

  vee::image d_img = vee::create_depth_image({width, height});
  vee::device_memory d_mem = vee::create_local_image_memory(pd, *d_img);
  vee::bind_image_memory(*d_img, *d_mem);
  vee::image_view d_iv = vee::create_depth_image_view(*d_img);

  vee::buffer o_buf = vee::create_transfer_dst_buffer(width * height * 4);
  vee::device_memory o_mem = vee::create_host_buffer_memory(pd, *o_buf);
  vee::bind_buffer_memory(*o_buf, *o_mem);

  vee::fb_params fbp{
      .physical_device = pd,
      .render_pass = *rp,
      .image_buffer = *t_iv,
      .depth_buffer = *d_iv,
      .extent = {width, height},
  };
  vee::framebuffer fb = vee::create_framebuffer(fbp);

  vee::command_pool cp = vee::create_command_pool(qf);
  vee::command_buffer cb = vee::allocate_primary_command_buffer(*cp);

  program p{pd, rp};

  {
    vee::begin_cmd_buf_one_time_submit(cb);
    {
      vee::cmd_begin_render_pass({
          .command_buffer = cb,
          .render_pass = *rp,
          .framebuffer = *fb,
          .extent = {width, height},
          .clear_color = {{0.1, 0.2, 0.3, 1.0}},
      });
      p.run(cb);
      vee::cmd_end_render_pass(cb);
    }
    vee::cmd_pipeline_barrier(cb, *t_img, vee::from_pipeline_to_host);
    vee::cmd_copy_image_to_buffer(cb, {width, height}, *t_img, *o_buf);
    vee::end_cmd_buf(cb);
  }

  vee::fence f = vee::create_fence_signaled();
  vee::queue_submit({
      .queue = q,
      .fence = *f,
      .command_buffer = cb,
  });

  vee::device_wait_idle();

  vee::mapmem mem{*o_mem};
  auto *data = static_cast<stbi::pixel *>(*mem);
  stbi::write_rgba_unsafe(filename, width, height, data);
  silog::log(silog::info, "output written to [%s]", filename);
}

#pragma leco add_shader "bait.vert"
#pragma leco add_shader "bait.frag"
#pragma leco tool
