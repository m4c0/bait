export module bait:offscreen;
import silog;
import stubby;
import traits;
import vee;

class offscreen_framebuffer {
  static constexpr const auto width = 1280;
  static constexpr const auto height = 768;
  static constexpr const auto filename = "out/test.jpg";

  vee::physical_device pd;

  // Colour buffer
  vee::image t_img = vee::create_renderable_image({width, height});
  vee::device_memory t_mem = vee::create_local_image_memory(pd, *t_img);
  decltype(nullptr) t_bind = vee::bind_image_memory(*t_img, *t_mem);
  vee::image_view t_iv = vee::create_srgba_image_view(*t_img);

  // Depth buffer
  vee::image d_img = vee::create_depth_image({width, height});
  vee::device_memory d_mem = vee::create_local_image_memory(pd, *d_img);
  decltype(nullptr) d_bind = vee::bind_image_memory(*d_img, *d_mem);
  vee::image_view d_iv = vee::create_depth_image_view(*d_img);

  // Host-readable output buffer
  vee::buffer o_buf = vee::create_transfer_dst_buffer(width * height * 4);
  vee::device_memory o_mem = vee::create_host_buffer_memory(pd, *o_buf);
  decltype(nullptr) o_bind = vee::bind_buffer_memory(*o_buf, *o_mem);

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
  vee::gr_pipeline gp{};

public:
  explicit offscreen_framebuffer(vee::physical_device pd) : pd{pd} {}

  [[nodiscard]] constexpr const auto aspect() const noexcept {
    return static_cast<float>(width) / static_cast<float>(height);
  }
  [[nodiscard]] constexpr const auto &render_pass() const noexcept {
    return rp;
  }

  void set_pipeline(vee::gr_pipeline &&g) { gp = traits::move(g); }

  void cmd_begin_render_pass(vee::command_buffer cb) {
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
  }

  void cmd_copy_to_buffer(vee::command_buffer cb) {
    vee::cmd_pipeline_barrier(cb, *t_img, vee::from_pipeline_to_host);
    vee::cmd_copy_image_to_buffer(cb, {width, height}, *t_img, *o_buf);
  }

  void write_buffer_to_file() {
    // Sync CPU+GPU
    vee::device_wait_idle();

    vee::mapmem mem{*o_mem};
    auto *data = static_cast<stbi::pixel *>(*mem);
    stbi::write_rgba_unsafe(filename, width, height, data);
    silog::log(silog::info, "output written to [%s]", filename);
  }
};
