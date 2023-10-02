export module bait:descriptors;
import silog;
import stubby;
import vee;

static constexpr const auto icon_left = "photoshop.jpg";
static constexpr const auto icon_right = "vim.png";

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

class desc_set {
  vee::physical_device pd;

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

public:
  explicit desc_set(vee::physical_device pd) : pd{pd} {
    vee::update_descriptor_set(dset, 0, left.iv(), *smp);
    vee::update_descriptor_set(dset, 1, right.iv(), *smp);
  }

  [[nodiscard]] const auto &layout() const { return dsl; }
  [[nodiscard]] auto operator*() const { return dset; }

  void cmd_prepare_images(vee::command_buffer cb) {
    left.run(cb);
    right.run(cb);
  }
};
