#pragma leco app

import casein;
import voo;
import vinyl;

struct app_stuff {
  voo::device_and_queue dq { "bait", casein::native_ptr };
} * gas;

struct sized_stuff {
} * gss;

static void on_start() {
  gas = new app_stuff {};
}
static void on_frame() {
  if (!gss) gss = new sized_stuff {};
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
