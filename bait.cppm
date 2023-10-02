module;
#define offsetof(t, d) __builtin_offsetof(t, d)

export module bait;
import :offscreen;
import silog;
import stubby;
import vee;

static constexpr const auto icon_left = "photoshop.jpg";
static constexpr const auto icon_right = "vim.png";

struct upc {
  float aspect;
  float time;
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

class base_pipeline_layout {
  vee::descriptor_set_layout::type dsl;

  // Generic pipeline stuff
  vee::shader_module vert =
      vee::create_shader_module_from_resource("bait.vert.spv");
  vee::shader_module frag =
      vee::create_shader_module_from_resource("bait.frag.spv");
  vee::pipeline_layout pl = vee::create_pipeline_layout(
      {dsl}, {vee::vert_frag_push_constant_range<upc>()});

public:
  explicit base_pipeline_layout(const vee::descriptor_set_layout &dsl)
      : dsl{*dsl} {}

  [[nodiscard]] auto operator*() const noexcept { return *pl; }

  [[nodiscard]] auto create_graphics_pipeline(const vee::render_pass &rp) {
    return vee::create_graphics_pipeline(
        *pl, *rp,
        {
            vee::pipeline_vert_stage(*vert, "main"),
            vee::pipeline_frag_stage(*frag, "main"),
        },
        {
            vee::vertex_input_bind(sizeof(point)),
        },
        {
            vee::vertex_attribute_vec2(0, 0),
        });
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

  // Inputs (vertices + instance)
  vee::buffer q_buf = vee::create_vertex_buffer(sizeof(quad));
  vee::device_memory q_mem = vee::create_host_buffer_memory(pd, sizeof(quad));
  vee::bind_buffer_memory(*q_buf, *q_mem, 0);
  {
    vee::mapmem mem{*q_mem};
    *static_cast<quad *>(*mem) = {};
  }

  // Textures
  icon left{icon_left, pd};
  icon right{icon_right, pd};

  // Descriptor set layout + pool
  vee::descriptor_set_layout dsl = vee::create_descriptor_set_layout(
      {vee::dsl_fragment_sampler(), vee::dsl_fragment_sampler()});

  vee::descriptor_pool dp =
      vee::create_descriptor_pool(1, {vee::combined_image_sampler(2)});
  vee::descriptor_set dset = vee::allocate_descriptor_set(*dp, *dsl);

  vee::sampler smp = vee::create_sampler(vee::linear_sampler);
  vee::update_descriptor_set(dset, 0, left.iv(), *smp);
  vee::update_descriptor_set(dset, 1, right.iv(), *smp);

  // Base pipeline layout
  base_pipeline_layout bpl{dsl};
  // Offscreen FB
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
      left.run(cb);
      right.run(cb);

      osfb.cmd_begin_render_pass(cb);

      vee::cmd_bind_descriptor_set(cb, *bpl, 0, dset);
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
