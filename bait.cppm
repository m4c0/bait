module;
#define offsetof(t, d) __builtin_offsetof(t, d)

export module bait;
import silog;
import stubby;
import vee;

static constexpr const auto width = 1280;
static constexpr const auto height = 768;
static constexpr const auto filename = "out/test.jpg";
static constexpr const auto icon_left = "photoshop.jpg";
static constexpr const auto icon_right = "vim.png";

struct upc {
  float aspect;
};

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
  float a = 1.0f;
};
struct area {
  float x;
  float y;
  float w;
  float h;
};
struct inst {
  area rect;
};
static constexpr long double operator""_u(long double a) { return a / 256.0f; };
struct all {
  quad q{};
  inst i[2]{
      inst{
          .rect = {-1.0, -1.0, 2.0, 2.0},
      },
  };
};

class icon {
  unsigned m_w;
  unsigned m_h;

  vee::buffer m_sbuf;
  vee::device_memory m_smem;

  vee::image m_img;
  vee::device_memory m_mem;
  vee::image_view m_iv;

public:
  icon(const char *file, vee::physical_device pd) {
    stbi::load(file)
        .map([this, pd](auto &&img) {
          m_w = img.width;
          m_h = img.height;

          m_sbuf = vee::create_transfer_src_buffer(m_w * m_h * 4);
          m_smem = vee::create_host_buffer_memory(pd, *m_sbuf);
          vee::bind_buffer_memory(*m_sbuf, *m_smem);

          m_img = vee::create_srgba_image({m_w, m_h});
          m_mem = vee::create_local_image_memory(pd, *m_img);
          vee::bind_image_memory(*m_img, *m_mem);
          m_iv = vee::create_srgba_image_view(*m_img);

          vee::mapmem m{*m_smem};
          auto *c = static_cast<unsigned char *>(*m);
          for (auto i = 0; i < m_w * m_h * 4; i++) {
            c[i] = (*img.data)[i];
          }
        })
        .take([](auto msg) {
          silog::log(silog::error, "Failed loading resource image: %s", msg);
        });
  }

  [[nodiscard]] auto iv() const noexcept { return *m_iv; }

  void run(vee::command_buffer cb) {
    vee::cmd_pipeline_barrier(cb, *m_img, vee::from_host_to_transfer);
    vee::cmd_copy_buffer_to_image(cb, {m_w, m_h}, *m_sbuf, *m_img);
    vee::cmd_pipeline_barrier(cb, *m_img, vee::from_transfer_to_fragment);
  }
};

extern "C" int main() {
  // Instance
  vee::instance i = vee::create_instance("bait");
  vee::debug_utils_messenger dbg = vee::create_debug_utils_messenger();
  auto [pd, qf] = vee::find_physical_device_with_universal_queue(nullptr);

  // Device
  vee::device d = vee::create_single_queue_device(pd, qf);
  vee::queue q = vee::get_queue_for_family(qf);

  // Colour buffer
  vee::image t_img = vee::create_renderable_image({width, height});
  vee::device_memory t_mem = vee::create_local_image_memory(pd, *t_img);
  vee::bind_image_memory(*t_img, *t_mem);
  vee::image_view t_iv = vee::create_srgba_image_view(*t_img);

  // Depth buffer
  vee::image d_img = vee::create_depth_image({width, height});
  vee::device_memory d_mem = vee::create_local_image_memory(pd, *d_img);
  vee::bind_image_memory(*d_img, *d_mem);
  vee::image_view d_iv = vee::create_depth_image_view(*d_img);

  // Host-readable output buffer
  vee::buffer o_buf = vee::create_transfer_dst_buffer(width * height * 4);
  vee::device_memory o_mem = vee::create_host_buffer_memory(pd, *o_buf);
  vee::bind_buffer_memory(*o_buf, *o_mem);

  // Renderpass + Framebuffer
  vee::render_pass rp = vee::create_render_pass(pd, nullptr);
  vee::fb_params fbp{
      .physical_device = pd,
      .render_pass = *rp,
      .image_buffer = *t_iv,
      .depth_buffer = *d_iv,
      .extent = {width, height},
  };
  vee::framebuffer fb = vee::create_framebuffer(fbp);

  // Inputs (vertices + instance)
  vee::buffer q_buf = vee::create_vertex_buffer(sizeof(all::q));
  vee::buffer i_buf = vee::create_vertex_buffer(sizeof(all::i));
  vee::device_memory iq_mem = vee::create_host_buffer_memory(pd, sizeof(all));
  vee::bind_buffer_memory(*q_buf, *iq_mem, 0);
  vee::bind_buffer_memory(*i_buf, *iq_mem, sizeof(quad));
  {
    vee::mapmem mem{*iq_mem};
    *static_cast<all *>(*mem) = {};
  }

  // Textures
  icon left{icon_left, pd};
  icon right{icon_right, pd};

  // Descriptor pool
  vee::descriptor_set_layout dsl = vee::create_descriptor_set_layout(
      {vee::dsl_fragment_sampler(), vee::dsl_fragment_sampler()});

  vee::descriptor_pool dp =
      vee::create_descriptor_pool(1, {vee::combined_image_sampler(2)});
  vee::descriptor_set dset = vee::allocate_descriptor_set(*dp, *dsl);

  vee::sampler smp = vee::create_sampler(vee::linear_sampler);
  vee::update_descriptor_set(dset, 0, left.iv(), *smp);
  vee::update_descriptor_set(dset, 1, right.iv(), *smp);

  // Pipeline
  vee::shader_module vert =
      vee::create_shader_module_from_resource("bait.vert.spv");
  vee::shader_module frag =
      vee::create_shader_module_from_resource("bait.frag.spv");
  vee::pipeline_layout pl = vee::create_pipeline_layout(
      {*dsl}, {vee::vert_frag_push_constant_range<upc>()});
  vee::gr_pipeline gp = vee::create_graphics_pipeline(
      *pl, *rp,
      {
          vee::pipeline_vert_stage(*vert, "main"),
          vee::pipeline_frag_stage(*frag, "main"),
      },
      {
          vee::vertex_input_bind(sizeof(point)),
          vee::vertex_input_bind_per_instance(sizeof(inst)),
      },
      {
          vee::vertex_attribute_vec2(0, 0),
          vee::vertex_attribute_vec4(1, offsetof(inst, rect)),
      });

  // Command pool + buffer
  vee::command_pool cp = vee::create_command_pool(qf);
  vee::command_buffer cb = vee::allocate_primary_command_buffer(*cp);

  // Build command buffer
  {
    upc pc{
        .aspect = static_cast<float>(width) / static_cast<float>(height),
    };

    vee::begin_cmd_buf_one_time_submit(cb);
    {
      left.run(cb);
      right.run(cb);

      vee::cmd_begin_render_pass({
          .command_buffer = cb,
          .render_pass = *rp,
          .framebuffer = *fb,
          .extent = {width, height},
          .clear_color = {{0.1, 0.2, 0.3, 1.0}},
          .use_secondary_cmd_buf = false,
      });
      vee::cmd_set_scissor(cb, {width, height});
      vee::cmd_set_viewport(cb, {width, height});

      vee::cmd_bind_gr_pipeline(cb, *gp);
      vee::cmd_bind_descriptor_set(cb, *pl, 0, dset);
      vee::cmd_push_vert_frag_constants(cb, *pl, &pc);

      vee::cmd_bind_vertex_buffers(cb, 0, *q_buf);
      vee::cmd_bind_vertex_buffers(cb, 1, *i_buf);
      vee::cmd_draw(cb, 6, sizeof(all::i) / sizeof(inst));
      vee::cmd_end_render_pass(cb);
    }
    vee::cmd_pipeline_barrier(cb, *t_img, vee::from_pipeline_to_host);
    vee::cmd_copy_image_to_buffer(cb, {width, height}, *t_img, *o_buf);
    vee::end_cmd_buf(cb);
  }

  // Submit
  vee::queue_submit({
      .queue = q,
      .command_buffer = cb,
  });

  // Sync CPU+GPU
  vee::device_wait_idle();

  // Pull data from buffer
  vee::mapmem mem{*o_mem};
  auto *data = static_cast<stbi::pixel *>(*mem);
  stbi::write_rgba_unsafe(filename, width, height, data);
  silog::log(silog::info, "output written to [%s]", filename);
}

#pragma leco add_shader "bait.vert"
#pragma leco add_shader "bait.frag"
#pragma leco tool
