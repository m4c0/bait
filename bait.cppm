#pragma leco tool
#pragma leco add_shader "bait.vert"
#pragma leco add_shader "bait.frag"

export module bait;
import vee;

static constexpr const auto width = 1024;
static constexpr const auto height = 768;

struct point {
  float x;
  float y;
  float z;
  float w;
};

extern "C" int main() {
  vee::instance i = vee::create_instance("bait");
  auto [pd, qf] = vee::find_physical_device_with_universal_queue(nullptr);

  vee::device d = vee::create_single_queue_device(pd, qf);
  vee::queue q = vee::get_queue_for_family(qf);

  vee::render_pass rp = vee::create_render_pass(pd, nullptr);

  vee::shader_module vert =
      vee::create_shader_module_from_resource("bait.vert.spv");
  vee::shader_module frag =
      vee::create_shader_module_from_resource("bait.frag.spv");
  vee::pipeline_layout pl = vee::create_pipeline_layout();
  vee::gr_pipeline gp =
      vee::create_graphics_pipeline(*pl, *rp,
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

  vee::buffer v_buf = vee::create_vertex_buffer(sizeof(point) * 3);
  vee::device_memory v_mem = vee::create_host_buffer_memory(pd, *v_buf);
  vee::bind_buffer_memory(*v_buf, *v_mem);

  vee::image t_img = vee::create_readable_srgba_image({width, height});
  vee::device_memory t_mem = vee::create_local_image_memory(pd, *t_img);
  vee::bind_image_memory(*t_img, *t_mem);
  vee::image_view t_iv = vee::create_srgba_image_view(*t_img);

  vee::image d_img = vee::create_depth_image({width, height});
  vee::device_memory d_mem = vee::create_local_image_memory(pd, *d_img);
  vee::bind_image_memory(*d_img, *d_mem);
  vee::image_view d_iv = vee::create_depth_image_view(*d_img);

  vee::buffer o_buf = vee::create_transfer_dst_buffer(width * height * 4);
  vee::device_memory o_mem = vee::create_host_buffer_memory(pd, *o_buf);
  vee::bind_buffer_memory(*o_buf, *o_mem);

  vee::fb_params fbp{
      .physical_device = pd,
      .render_pass = *rp,
      .image_buffer = *t_iv,
      .depth_buffer = *d_iv,
      .extent = {width, height},
  };
  vee::framebuffer fb = vee::create_framebuffer(fbp);

  vee::command_pool cp = vee::create_command_pool(qf);
  vee::command_buffer cb = vee::allocate_primary_command_buffer(*cp);
}
