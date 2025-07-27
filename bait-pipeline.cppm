export module bait:pipeline;
import vee;
import voo;

struct upc {
  float aspect;
  float time;
};

class base_pipeline_layout {
  vee::descriptor_set_layout::type dsl;

  vee::pipeline_layout pl = vee::create_pipeline_layout(
      dsl,
      vee::vert_frag_push_constant_range<upc>());

public:
  explicit base_pipeline_layout(const vee::descriptor_set_layout &dsl)
      : dsl{*dsl} {}

  [[nodiscard]] auto operator*() const noexcept { return *pl; }

  [[nodiscard]] auto create_graphics_pipeline(const vee::render_pass::type rp) {
    return vee::create_graphics_pipeline({
        .pipeline_layout = *pl,
        .render_pass = rp,
        .shaders{
            voo::shader { "bait.vert.spv" }.pipeline_vert_stage(),
            voo::shader { "bait.frag.spv" }.pipeline_frag_stage(),
        },
        .bindings{
            vee::vertex_input_bind(sizeof(float) * 2),
        },
        .attributes{
            vee::vertex_attribute_vec2(0, 0),
        },
    });
  }
};
