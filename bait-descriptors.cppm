#pragma leco add_resource "cpp.png"
#pragma leco add_resource "lambda.png"
export module bait:descriptors;
import jute;
import vee;
import voo;

static constexpr const jute::view icon_left = "cpp.png";
static constexpr const jute::view icon_right = "lambda.png";

class desc_set {
  // Textures
  voo::bound_image left;
  voo::bound_image right;

  // Descriptor set layout + pool
  vee::descriptor_set_layout dsl = vee::create_descriptor_set_layout(
      {vee::dsl_fragment_sampler(), vee::dsl_fragment_sampler()});

  vee::descriptor_pool dp =
      vee::create_descriptor_pool(1, {vee::combined_image_sampler(2)});
  vee::descriptor_set dset = vee::allocate_descriptor_set(*dp, *dsl);

  vee::sampler smp = vee::create_sampler(vee::linear_sampler);

public:
  explicit desc_set(voo::device_and_queue * dq) {
    voo::load_image(icon_left, dq->physical_device(), dq->queue(), &left, [this] {
      vee::update_descriptor_set(dset, 0, *left.iv, *smp);
    }); 
    voo::load_image(icon_right, dq->physical_device(), dq->queue(), &right, [this] {
      vee::update_descriptor_set(dset, 1, *right.iv, *smp);
    }); 
  }

  [[nodiscard]] const auto &layout() const { return dsl; }
  [[nodiscard]] auto operator*() const { return dset; }
};
