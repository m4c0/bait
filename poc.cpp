#pragma leco app
#pragma leco add_shader "poc.frag"
#pragma leco add_shader "poc.vert"
#pragma leco add_resource_dir assets

import casein;
import dotz;
import hai;
import natty;
import stubby;
import sv;
import voo;
import vinyl;

struct upc {
  dotz::vec2 aa;
  dotz::vec2 bb;
  dotz::vec2 uva;
  dotz::vec2 uvb;
  dotz::vec2 scale;
};

struct call {
  hai::cstr font {};
  unsigned size {};
  hai::cstr text {};
  dotz::vec2 pos {};
};
struct box {
  dotz::vec2 pos {};
  dotz::vec2 size {};
  unsigned colour {};
};
struct img {
  dotz::vec2 pos {};
  dotz::vec2 size {};
  sv file;
};
struct model {
  hai::varray<img> images { 128 };
  hai::varray<call> calls { 128 };
  hai::varray<box> boxes { 128 };
} gmdl = [] {
  model res {};
  res.images.push_back(img {
    .pos { -512 },
    .size { 1024 },
    .file = "napster.jpg"_sv,
  });
  // res.images.push_back(img {
  //   .pos { 100, -400 },
  //   .size { 300, 500 },
  //   .file = "unity.png"_sv,
  // });
  res.images.push_back(img {
    .pos { -100, -100 },
    .size { 200, 340 },
    .file = "m4c0.jpg"_sv,
  });
  // res.boxes.push_back(box {
  //   .pos { -1024 },
  //   .size { 2048 },
  //   .colour = 0xC0000000,
  // });
  //res.calls.push_back(call {
  //  .font { "Impact"_sv },
  //  .size = 200,
  //  .text { "Sonzera!"_sv },
  //  .pos { -300, 240 },
  //});
  //res.boxes.push_back(box {
  //  .pos { -1275, -195 },
  //  .size { 1280, 400 },
  //  .colour = 0xFF000000,
  //});
  //res.boxes.push_back(box {
  //  .pos { -1280, -200 },
  //  .size { 1280, 400 },
  //  .colour = 0xFF0000FF,
  //});
  //res.calls.push_back(call {
  //  .font { "DIN Condensed"_sv },
  //  .size = 128,
  //  .text { "Trabalhando"_sv },
  //  .pos { -768, -156 },
  //});
  //res.calls.push_back(call {
  //  .font { "DIN Condensed"_sv },
  //  .size = 128,
  //  .text { "nas férias?"_sv },
  //  .pos { -768, -30 },
  //});
  //res.calls.push_back(call {
  //  .font { "Futura"_sv },
  //  .size = 48,
  //  .text { "com m4c0"_sv },
  //  .pos { -256, 130 },
  //});
  return res;
}();

class colour_image {
  voo::h2l_image m_img {};
  voo::single_frag_dset m_dset { 1 };
  voo::single_cb m_cb {};
  vee::sampler m_smp = vee::create_sampler(vee::linear_sampler);

public:
  void load(vee::physical_device pd, unsigned colour) {
    m_img = voo::h2l_image {{
      .pd = pd,
      .w = 16,
      .h = 16,
      .fmt = VK_FORMAT_R8G8B8A8_SRGB,
    }};

    {
      voo::memiter<unsigned> pixies { m_img.host_memory() };
      for (auto i = 0; i < 256; i++) pixies += colour;
    }
    vee::update_descriptor_set(m_dset.descriptor_set(), 0, 0, m_img.iv(), *m_smp);
    
    {
      voo::cmd_buf_one_time_submit ots { m_cb.cb() };
      m_img.setup_copy(m_cb.cb());
    }
    voo::queue::universal()->queue_submit({
      .command_buffer = m_cb.cb(),
    });
  }

  [[nodiscard]] constexpr auto descriptor_set() const { return m_dset.descriptor_set(); }
};

class file_image {
  voo::bound_image m_img {};
  voo::single_frag_dset m_dset { 1 };
  vee::sampler m_smp = vee::create_sampler(vee::linear_sampler);

public:
  void load(vee::physical_device pd, sv file) {
    voo::load_image(file, pd, &m_img, [this](auto) {
      vee::update_descriptor_set(m_dset.descriptor_set(), 0, 0, *m_img.iv, *m_smp);
    });
  }

  [[nodiscard]] constexpr auto descriptor_set() const { return m_dset.descriptor_set(); }
};

static auto create_pipeline(vee::pipeline_layout::type pl, vee::render_pass::type rp) {
  return vee::create_graphics_pipeline({
    .pipeline_layout = pl,
    .render_pass = rp,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    .shaders {
      voo::shader("poc.vert.spv").pipeline_vert_stage(),
      voo::shader("poc.frag.spv").pipeline_frag_stage(),
    },
  });
}

struct app_stuff {
  voo::device_and_queue dq { "bait", casein::native_ptr };
  vee::render_pass rp = voo::single_att_render_pass(dq.physical_device(), dq.surface());

  voo::single_frag_dset dset_text { 1 };

  vee::descriptor_set_layout dsl = vee::create_descriptor_set_layout({
    vee::dsl_fragment_sampler()
  });
  vee::pipeline_layout pl = vee::create_pipeline_layout(
      *dsl,
      vee::vertex_push_constant_range<upc>());
  vee::gr_pipeline gp = create_pipeline(*pl, *rp);

  vee::sampler smp = vee::create_sampler(vee::linear_sampler);

  hai::array<colour_image> bars {};
  hai::array<file_image> images {};
  voo::h2l_image text {{
    .pd = dq.physical_device(),
    .w = 1024,
    .h = 1024,
    .fmt = VK_FORMAT_R8G8B8A8_SRGB,
  }}; 
};
static hai::uptr<app_stuff> gas {};

struct sized_stuff {
  voo::swapchain_and_stuff sw { gas->dq, *gas->rp };
};
static hai::uptr<sized_stuff> gss {};

static void on_start() {
  gas.reset(new app_stuff {});

  gas->images.set_capacity(gmdl.images.size());
  for (auto i = 0; i < gmdl.images.size(); i++) {
    gas->images[i].load(gas->dq.physical_device(), gmdl.images[i].file);
  }

  gas->bars.set_capacity(gmdl.boxes.size());
  for (auto i = 0; i < gmdl.boxes.size(); i++) {
    gas->bars[i].load(gas->dq.physical_device(), gmdl.boxes[i].colour);
  }

  {
    natty::surface_t surf = natty::create_surface(gas->text.width(), gas->text.height());

    int position = 0;
    for (auto & call : gmdl.calls) {
      natty::font_t font = natty::create_font(call.font.begin(), call.size);
      natty::draw({
        .surface = *surf,
        .font = *font,
        .position { 0, position },
        .text = call.text,
      });
      position += call.size * 1.25;
    }

    voo::memiter<unsigned> pixies { gas->text.host_memory() };
    auto ptr = natty::surface_data(*surf).begin();
    for (auto i = 0; i < gas->text.width() * gas->text.height(); i++) {
      pixies += *ptr++;
    }
  }
  voo::single_cb scb {};
  {
    voo::cmd_buf_one_time_submit ots { scb.cb() };
    gas->text.setup_copy(scb.cb());
  }
  voo::queue::universal()->queue_submit({ .command_buffer = scb.cb() });
  voo::queue::universal()->device_wait_idle();

  vee::update_descriptor_set(gas->dset_text.descriptor_set(), 0, 0, gas->text.iv(), *gas->smp);

}

static void render(vee::command_buffer cb, float a) {
  dotz::vec2 aspect { a, 1.f };

  upc pc {
    .aa = dotz::vec2 { -512.f } * aspect,
    .bb = dotz::vec2 { 512.f } * aspect,
    .uva = dotz::vec2 {},
    .uvb = dotz::vec2 { 1024.f },
    .scale = dotz::vec2 { 512.f } * aspect,
  };

  for (auto i = 0; i < gmdl.images.size(); i++) {
    auto & mdl = gmdl.images[i];
    auto & img = gas->images[i];
    pc.aa = mdl.pos * aspect;
    pc.bb = (mdl.pos + mdl.size) * aspect;
    vee::cmd_push_vertex_constants(cb, *gas->pl, &pc);
    vee::cmd_bind_descriptor_set(cb, *gas->pl, 0, img.descriptor_set());
    vee::cmd_draw(cb, 6, 1);
  }

  for (auto i = 0; i < gmdl.boxes.size(); i++) {
    auto & box = gmdl.boxes[i];
    auto & img = gas->bars[i];
    pc = {
      .aa = box.pos,
      .bb = box.pos + box.size,
      .scale = pc.scale,
    };
    vee::cmd_push_vertex_constants(cb, *gas->pl, &pc);
    vee::cmd_bind_descriptor_set(cb, *gas->pl, 0, img.descriptor_set());
    vee::cmd_draw(cb, 6, 1);
  }

  int tpos = 0;
  for (auto & call : gmdl.calls) {
    auto pos = call.pos;
    int size = call.size * 1.25;
    pc = {
      .aa = pos,
      .bb = pos + dotz::vec2 { 1024, size },
      .uva { 0, tpos },
      .uvb { 1024, tpos + size },
      .scale = pc.scale,
    };
    vee::cmd_push_vertex_constants(cb, *gas->pl, &pc);
    vee::cmd_bind_descriptor_set(cb, *gas->pl, 0, gas->dset_text.descriptor_set());
    vee::cmd_draw(cb, 6, 1);
    tpos += size;
  }
}

static void on_frame() {
  if (!gss) gss.reset(new sized_stuff {});

  gss->sw.acquire_next_image();
  gss->sw.queue_one_time_submit([] {
    auto cb = gss->sw.command_buffer();
    auto rp = gss->sw.cmd_render_pass(vee::render_pass_begin {
      .command_buffer = cb,
    });
    vee::cmd_bind_gr_pipeline(cb, *gas->gp);
    vee::cmd_set_viewport(cb, gss->sw.extent());
    vee::cmd_set_scissor(cb, gss->sw.extent());
    render(cb, gss->sw.aspect());
  });
  gss->sw.queue_present();
}
static void on_resize() { gss = {}; }
static void on_stop() { gss = {}; gas = {}; }

const auto i = [] {
  using namespace vinyl;
  on(START, on_start);
  on(FRAME, on_frame);
  on(RESIZE, on_resize);
  on(STOP, on_stop);

  using namespace casein;
  handle(KEY_DOWN, K_SPACE, [] {
    voo::single_cb scb {};
    auto cb = scb.cb();

    vee::extent ext { 1280, 720 };
    voo::offscreen::buffers ofs { gas->dq.physical_device(), ext, VK_FORMAT_R8G8B8A8_SRGB };
    vee::gr_pipeline gp = create_pipeline(*gas->pl, ofs.render_pass());

    {
      voo::cmd_buf_one_time_submit ots { cb };
      {
        voo::cmd_render_pass crp { ofs.render_pass_begin({
          .command_buffer = cb,
          .clear_colours {
            vee::clear_colour(1, 1, 1, 1),
            vee::clear_depth(0),
          },
        }), true };
        vee::cmd_set_viewport(cb, ext);
        vee::cmd_set_scissor(cb, ext);
        vee::cmd_bind_gr_pipeline(cb, *gp);
        render(cb, 1280.f / 720.f);
      }
      ofs.cmd_copy_to_host(cb);
    }
    voo::queue::universal()->queue_submit({ .command_buffer = cb });
    voo::queue::universal()->device_wait_idle();

    auto mem = ofs.map_host();
    auto * data = static_cast<stbi::pixel *>(*mem);
    stbi::write_rgba_unsafe("out/shot.png", ext.width, ext.height, data);
  });

  return 0;
}();
