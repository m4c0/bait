export module bait:surface;
import :pipeline;
import hai;
import traits;
import vee;

class surface_framebuffer {
  vee::render_pass rp;
  voo::swapchain_and_stuff sw;
  vee::gr_pipeline gp{};

  upc pc;

public:
  surface_framebuffer(const voo::device_and_queue & dq) :
    rp { vee::create_render_pass({
      .attachments {{
        vee::create_colour_attachment(dq.physical_device(), dq.surface()),
      }},
      .subpasses {{
        vee::create_subpass({
          .colours {{ vee::create_attachment_ref(0, vee::image_layout_color_attachment_optimal) }},
        }),
      }},
      .dependencies {{
        vee::create_colour_dependency(),
      }},
    }) }
  , sw { dq, *rp }
  , pc { dq.aspect_of() }
  {}

  [[nodiscard]] constexpr auto &push_constants() noexcept { return pc; }
  [[nodiscard]] constexpr const auto &render_pass() const noexcept {
    return rp;
  }

  [[nodiscard]] constexpr auto cb() const { return sw.command_buffer(); }

  void set_pipeline(vee::gr_pipeline &&g) { gp = traits::move(g); }

  void cmd_begin_render_pass(vee::command_buffer cb) {
    vee::cmd_begin_render_pass(sw.render_pass_begin({
        .clear_colours { vee::clear_colour(0.1, 0.2, 0.3, 1.0) },
        .use_secondary_cmd_buf = false,
    }));
    vee::cmd_set_scissor(cb, sw.extent());
    vee::cmd_set_viewport(cb, sw.extent());
    vee::cmd_bind_gr_pipeline(cb, *gp);
  }

  void wait_reset_and_acquire() {
    sw.acquire_next_image();
  }
  void submit_and_present(voo::queue * q) {
    sw.queue_submit(q);
    sw.queue_present(q);
  }
};
