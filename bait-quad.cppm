export module bait:quad;
import vee;
import voo;

struct point {
  float x;
  float y;
};

struct quad {
  point p[6]{
      {0.0, 0.0}, {1.0, 1.0}, {1.0, 0.0},

      {1.0, 1.0}, {0.0, 0.0}, {0.0, 1.0},
  };
};

class quad_buf {
  vee::physical_device pd;

  // Inputs (vertices + instance)
  vee::buffer q_buf = vee::create_vertex_buffer(sizeof(quad));
  vee::device_memory q_mem = vee::create_host_buffer_memory(pd, sizeof(quad));

public:
  explicit quad_buf(vee::physical_device pd) : pd{pd} {
    vee::bind_buffer_memory(*q_buf, *q_mem, 0);
    voo::mapmem mem{*q_mem};
    *static_cast<quad *>(*mem) = {};
  }

  [[nodiscard]] auto operator*() const { return *q_buf; }
};
