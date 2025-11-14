#pragma leco app
#pragma leco add_shader "poc.frag"
#pragma leco add_shader "poc.vert"
#pragma leco add_resource_dir assets

import casein;
import dotz;
import hai;
import natty;
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
struct model {
  hai::varray<call> calls { 128 };
  hai::cstr image {};
  unsigned colour {};
} gmdl = [] {
  model res {
    .image { "beach.png"_sv },
    .colour = 0xFF0000FF,
  };
  res.calls.push_back(call {
    .font { "DIN Condensed"_sv },
    .size = 128,
    .text { "Trabalhando"_sv },
    .pos { -768, -156 },
  });
  res.calls.push_back(call {
    .font { "DIN Condensed"_sv },
    .size = 128,
    .text { "nas férias?"_sv },
    .pos { -768, -30 },
  });
  res.calls.push_back(call {
    .font { "Futura"_sv },
    .size = 48,
    .text { "com m4c0"_sv },
    .pos { -256, 130 },
  });
  return res;
}();

struct app_stuff {
  voo::device_and_queue dq { "bait", casein::native_ptr };
  vee::render_pass rp = voo::single_att_render_pass(dq.physical_device(), dq.surface());

  voo::single_frag_dset dset { 1 };
  voo::single_frag_dset dset_text { 1 };
  voo::single_frag_dset dset_bar { 1 };

  vee::pipeline_layout pl = vee::create_pipeline_layout(
      dset.descriptor_set_layout(),
      vee::vertex_push_constant_range<upc>());
  vee::gr_pipeline gp = vee::create_graphics_pipeline({
    .pipeline_layout = *pl,
    .render_pass = *rp,
    .shaders {
      voo::shader("poc.vert.spv").pipeline_vert_stage(),
      voo::shader("poc.frag.spv").pipeline_frag_stage(),
    },
    .bindings {
      vee::vertex_input_bind(sizeof(dotz::vec2)),
    },
    .attributes {
      vee::vertex_attribute_vec2(0, 0),
    },
  });

  vee::sampler smp = vee::create_sampler(vee::linear_sampler);

  voo::one_quad quad { dq.physical_device() };

  voo::bound_image back;
  voo::h2l_image text {{
    .pd = dq.physical_device(),
    .w = 1024,
    .h = 1024,
    .fmt = VK_FORMAT_R8G8B8A8_SRGB,
  }}; 
  voo::h2l_image bar {{
    .pd = dq.physical_device(),
    .w = 16,
    .h = 16,
    .fmt = VK_FORMAT_R8G8B8A8_SRGB,
  }}; 
} * gas;

struct sized_stuff {
  voo::swapchain_and_stuff sw { gas->dq, *gas->rp };
} * gss;

static void on_start() {
  gas = new app_stuff {};

  voo::load_image(gmdl.image, gas->dq.physical_device(), gas->dq.queue(), &gas->back, [] {
    vee::update_descriptor_set(gas->dset.descriptor_set(), 0, 0, *gas->back.iv, *gas->smp);
  });

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
  vee::update_descriptor_set(gas->dset_text.descriptor_set(), 0, 0, gas->text.iv(), *gas->smp);

  {
    voo::memiter<unsigned> pixies { gas->bar.host_memory() };
    for (auto i = 0; i < 256; i++) pixies += gmdl.colour;
  }
  vee::update_descriptor_set(gas->dset_bar.descriptor_set(), 0, 0, gas->bar.iv(), *gas->smp);
}
static void on_frame() {
  if (!gss) gss = new sized_stuff {};

  gss->sw.acquire_next_image();
  gss->sw.queue_one_time_submit(gas->dq.queue(), [] {
    auto cb = gss->sw.command_buffer();
    gas->text.setup_copy(gss->sw.command_buffer());
    gas->bar.setup_copy(gss->sw.command_buffer());

    dotz::vec2 aspect { gss->sw.aspect(), 1.f };

    upc pc {
      .aa = dotz::vec2 { -512.f } * aspect,
      .bb = dotz::vec2 { 512.f } * aspect,
      .uva = dotz::vec2 {},
      .uvb = dotz::vec2 { 1024.f },
      .scale = dotz::vec2 { 512.f } * aspect,
    };

    auto rp = gss->sw.cmd_render_pass({
      .command_buffer = gss->sw.command_buffer(),
    });
    vee::cmd_set_viewport(cb, gss->sw.extent());
    vee::cmd_set_scissor(cb, gss->sw.extent());

    vee::cmd_push_vertex_constants(cb, *gas->pl, &pc);
    vee::cmd_bind_descriptor_set(cb, *gas->pl, 0, gas->dset.descriptor_set());
    vee::cmd_bind_gr_pipeline(cb, *gas->gp);
    gas->quad.run(cb, 0);

    pc = {
      .aa { -1280, -200 },
      .bb { 0, 200 },
      .scale = pc.scale,
    };
    vee::cmd_push_vertex_constants(cb, *gas->pl, &pc);
    vee::cmd_bind_descriptor_set(cb, *gas->pl, 0, gas->dset_bar.descriptor_set());
    vee::cmd_bind_gr_pipeline(cb, *gas->gp);
    gas->quad.run(cb, 0);

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
      vee::cmd_bind_gr_pipeline(cb, *gas->gp);
      gas->quad.run(cb, 0);
      tpos += size;
    }
  });
  gss->sw.queue_present(gas->dq.queue());
}
static void on_resize() {
  if (gss) delete gss;
}
static void on_stop() {
  if (gss) delete gss;
  if (gas) delete gas;
}

const auto i = [] {
  using namespace vinyl;
  on(START, on_start);
  on(FRAME, on_frame);
  on(RESIZE, on_resize);
  on(STOP, on_stop);

  return 0;
}();
