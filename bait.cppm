#pragma leco tool
export module bait;
import vee;

extern "C" int main() {
  vee::instance i = vee::create_instance("bait");
  vee::physical_device_pair pdqf =
      vee::find_physical_device_with_universal_queue(nullptr);
  vee::device d =
      vee::create_single_queue_device(pdqf.physical_device, pdqf.queue_family);
  vee::queue q = vee::get_queue_for_family(pdqf.queue_family);
}
