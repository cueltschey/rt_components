#ifndef RF_BASE_H
#define RF_BASE_H

#include "args.h"
#include <memory>

typedef enum uuagent_error_enum {
  UUAGENT_SUCCESS = 0,
  UUAGENT_FILE_ERROR,
  UUAGENT_UHD_ERROR,
  UUAGENT_ZMQ_ERROR,
  UUAGENT_SAMPLE_ERROR,
  UUAGENT_INVALID_RF_TYPE
} uuagent_error_e;

class RFBase {
  public:
    virtual ~RFBase() = default;
    virtual uuagent_error_e collect_iq_data(const all_args_t& args) = 0;
};

// Factory function declaration
std::unique_ptr<RFBase> create_rf_instance(const std::string& rf_type);

#endif  // !RF_BASE_H
