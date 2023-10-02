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

  void set_pipeline(vee::gr_pipeline &&g) { gp = traits::move(gp); }
};
