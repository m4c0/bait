export module bait:pipeline;
import vee;

struct upc {
  float aspect;
  float time;
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
            vee::vertex_input_bind(sizeof(float) * 2),
        },
        {
            vee::vertex_attribute_vec2(0, 0),
        });
  }
};
