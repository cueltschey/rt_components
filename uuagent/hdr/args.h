#ifndef ARGS_H
#define ARGS_H

#include <cstddef>
#include <string>

typedef struct rf_args_s {
  std::string rf_type;
  std::string device_args;
  std::string output_file;
  double srate_hz;
  float rx_gain;
  float tx_gain;
  float rx_freq;
  float tx_freq;
  size_t num_samples;
} rf_args_t;

typedef struct all_args_s {
  rf_args_t rf;
} all_args_t;

#endif // !ARGS_H
