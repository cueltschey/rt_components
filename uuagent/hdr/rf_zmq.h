#ifndef RF_ZMQ_H
#define RF_ZMQ_H

#include "rf_base.h"
#include <zmq.hpp>

class RF_ZMQ : public RFBase {
public:
  RF_ZMQ();
  ~RF_ZMQ();
  uuagent_error_e collect_iq_data(const all_args_t& args) override;

private:
  zmq::context_t context;
  zmq::socket_t socket;
};

#endif // !RF_ZMQ_H
