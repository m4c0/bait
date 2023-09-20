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
  float z;
  float w;
};

class triangle {
  vee::physical_device m_pd;

  vee::buffer m_buf = vee::create_vertex_buffer(sizeof(point) * 3);
  vee::device_memory m_mem = vee::create_host_buffer_memory(m_pd, *m_buf);
  decltype(nullptr) m_bind = vee::bind_buffer_memory(*m_buf, *m_mem);

public:
  triangle(vee::physical_device pd) : m_pd{pd} {
    vee::mapmem mem{*m_mem};
    auto *pix = static_cast<point *>(*mem);
    pix[0] = {-1.0, -1.0, 0.0, 0.0};
    pix[1] = {1.0, 1.0, 0.0, 0.0};
    pix[2] = {1.0, -1.0, 0.0, 0.0};
  }

  [[nodiscard]] constexpr auto buf() const noexcept { return *m_buf; }
};

class pipeline {
  const vee::render_pass &m_rp;

  vee::shader_module m_vert =
      vee::create_shader_module_from_resource("bait.vert.spv");
  vee::shader_module m_frag =
      vee::create_shader_module_from_resource("bait.frag.spv");
  vee::pipeline_layout m_pl = vee::create_pipeline_layout();
  vee::gr_pipeline m_gp = vee::create_graphics_pipeline(
      *m_pl, *m_rp,
      {
          vee::pipeline_vert_stage(*m_vert, "main"),
          vee::pipeline_frag_stage(*m_frag, "main"),
      },
      {
          vee::vertex_input_bind(sizeof(point)),
      },
      {
          vee::vertex_attribute_vec2(0, 0),
      });

public:
  pipeline(const vee::render_pass &rp) : m_rp{rp} {}

  [[nodiscard]] constexpr auto ppl() const noexcept { return *m_gp; }
};

class program {
  triangle m_t;
  pipeline m_p;

public:
  program(const vee::physical_device pd, const vee::render_pass &rp)
      : m_t{pd}, m_p{rp} {}

  void run(vee::command_buffer cb) {
    vee::cmd_bind_gr_pipeline(cb, m_p.ppl());
    vee::cmd_bind_vertex_buffers(cb, 0, m_t.buf());
    vee::cmd_draw(cb, 3);
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
