export module bait:offscreen;
import :pipeline;
import silog;
import stubby;
import traits;
import vee;
import voo;

class offscreen_framebuffer {
  static constexpr const auto width = 1280;
  static constexpr const auto height = 768;
  static constexpr const auto filename = "out/test.jpg";
  static constexpr const vee::extent extent { width, height };

  vee::physical_device pd;
  voo::offscreen::buffers ofs { pd, extent, VK_FORMAT_R8G8B8A8_SRGB };

  vee::gr_pipeline gp{};

  upc pc{
      .aspect = static_cast<float>(width) / static_cast<float>(height),
  };

public:
  explicit offscreen_framebuffer(vee::physical_device pd) : pd{pd} {}

  [[nodiscard]] constexpr auto &push_constants() noexcept { return pc; }
  [[nodiscard]] constexpr auto render_pass() const noexcept {
    return ofs.render_pass();
  }

  void set_pipeline(vee::gr_pipeline &&g) { gp = traits::move(g); }

  void cmd_begin_render_pass(vee::command_buffer cb) {
    vee::cmd_begin_render_pass(ofs.render_pass_begin({
        .command_buffer = cb,
        .clear_colours { vee::clear_colour(0.1, 0.2, 0.3, 1.0) },
    }));
    vee::cmd_set_scissor(cb, extent);
    vee::cmd_set_viewport(cb, extent);
    vee::cmd_bind_gr_pipeline(cb, *gp);
  }

  void cmd_copy_to_buffer(vee::command_buffer cb) {
    ofs.cmd_copy_to_host(cb);
  }

  void write_buffer_to_file() {
    // Sync CPU+GPU
    vee::device_wait_idle();

    auto mem = ofs.map_host();
    auto *data = static_cast<stbi::pixel *>(*mem);
    stbi::write_rgba_unsafe(filename, width, height, data);
    silog::log(silog::info, "output written to [%s]", filename);
  }
};
