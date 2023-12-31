export module bait:surface;
import :pipeline;
import hai;
import traits;
import vee;

class surface_framebuffer {
  vee::physical_device pd;
  vee::surface::type s;

  // Sync stuff
  vee::semaphore img_available_sema = vee::create_semaphore();
  vee::semaphore rnd_finished_sema = vee::create_semaphore();
  vee::fence f = vee::create_fence_signaled();

  // Depth buffer
  vee::image d_img = vee::create_depth_image(pd, s);
  vee::device_memory d_mem = vee::create_local_image_memory(pd, *d_img);
  [[maybe_unused]] decltype(nullptr) d_bind =
      vee::bind_image_memory(*d_img, *d_mem);
  vee::image_view d_iv = vee::create_depth_image_view(*d_img);

  vee::swapchain swc = vee::create_swapchain(pd, s);
  vee::extent ext = vee::get_surface_capabilities(pd, s).currentExtent;
  vee::render_pass rp = vee::create_render_pass(pd, s);

  hai::array<vee::image_view> c_ivs;
  hai::array<vee::framebuffer> fbs;
  vee::gr_pipeline gp{};

  upc pc{
      .aspect = static_cast<float>(ext.width) / static_cast<float>(ext.height),
  };

public:
  surface_framebuffer(vee::physical_device pd, const vee::surface &s)
      : pd{pd}, s{*s} {
    auto swc_imgs = vee::get_swapchain_images(*swc);
    c_ivs = hai::array<vee::image_view>{swc_imgs.size()};
    fbs = hai::array<vee::framebuffer>{swc_imgs.size()};

    for (auto i = 0; i < swc_imgs.size(); i++) {
      c_ivs[i] = vee::create_rgba_image_view(swc_imgs[i], pd, *s);
      fbs[i] = vee::create_framebuffer({
          .physical_device = pd,
          .surface = *s,
          .render_pass = *rp,
          .image_buffer = *c_ivs[i],
          .depth_buffer = *d_iv,
      });
    }
  }

  [[nodiscard]] constexpr auto &push_constants() noexcept { return pc; }
  [[nodiscard]] constexpr const auto &render_pass() const noexcept {
    return rp;
  }

  void set_pipeline(vee::gr_pipeline &&g) { gp = traits::move(g); }

  void cmd_begin_render_pass(vee::command_buffer cb, auto idx) {
    vee::cmd_begin_render_pass({
        .command_buffer = cb,
        .render_pass = *rp,
        .framebuffer = *fbs[idx],
        .extent = ext,
        .clear_color = {{0.1, 0.2, 0.3, 1.0}},
        .use_secondary_cmd_buf = false,
    });
    vee::cmd_set_scissor(cb, ext);
    vee::cmd_set_viewport(cb, ext);
    vee::cmd_bind_gr_pipeline(cb, *gp);
  }

  auto wait_reset_and_acquire() {
    vee::wait_and_reset_fence(*f);
    return vee::acquire_next_image(*swc, *img_available_sema);
  }
  void submit_and_present(vee::queue q, vee::command_buffer cb, auto idx) {
    vee::queue_submit({
        .queue = q,
        .fence = *f,
        .command_buffer = cb,
        .wait_semaphore = *img_available_sema,
        .signal_semaphore = *rnd_finished_sema,
    });
    vee::queue_present({
        .queue = q,
        .swapchain = *swc,
        .wait_semaphore = *rnd_finished_sema,
        .image_index = idx,
    });
  }
};
