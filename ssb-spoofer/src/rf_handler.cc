/**
 * SSB Spoofer RF Handler
 */

#include "rf_handler.h"
#include <iostream>
#include <cstring>

namespace ssb_spoofer {

RfHandler::RfHandler() : initialized_(false) {
  // Properly zero-initialize the RF device struct
  std::memset(&rf_device_, 0, sizeof(srsran_rf_t));
}

RfHandler::~RfHandler() {
  if (initialized_) {
      srsran_rf_close(&rf_device_);
  }
}

bool RfHandler::init(const RfConfig& config) {
  if (initialized_) {
      std::cerr << "RF Handler already initialized" << std::endl;
      return false;
  }
  
  config_ = config;
  
  // Build device args string with configured device arguments
  std::string args = config.device_args;
  char args_cstr[256];
  std::snprintf(args_cstr, sizeof(args_cstr), "%s", args.c_str());
  
  // Load RF plugins
  srsran_rf_load_plugins();
  
  // Open RF device using srsran_rf_open (exactly like working srsRAN tool)
  std::cout << "Opening RF device..." << std::endl;
  
  if (srsran_rf_open(&rf_device_, args_cstr) != SRSRAN_SUCCESS) {
      std::cerr << "Error opening RF device" << std::endl;
      return false;
  }
  
  // Set RX gain first (like srsRAN tool)
  std::cout << "Setting RX gain: " << config.rx_gain_db << " dB" << std::endl;
  if (srsran_rf_set_rx_gain(&rf_device_, config.rx_gain_db) != SRSRAN_SUCCESS) {
      std::cerr << "Error setting RX gain" << std::endl;
      srsran_rf_close(&rf_device_);
      return false;
  }
  
  // Set RX sample rate and print result (like srsRAN tool)
  std::cout << "Setting RX sample rate: " << config.srate_hz / 1e6 << " MHz" << std::endl;
  double actual_srate_rx = srsran_rf_set_rx_srate(&rf_device_, config.srate_hz);
  std::cout << "  Actual RX sample rate: " << actual_srate_rx / 1e6 << " MHz" << std::endl;
  
  // Set TX sample rate
  double actual_srate_tx = srsran_rf_set_tx_srate(&rf_device_, config.srate_hz);
  
  // Set RX frequency and print result (like srsRAN tool)
  std::cout << "Setting RX frequency: " << config.rx_freq_hz / 1e6 << " MHz" << std::endl;
  double actual_rx_freq = srsran_rf_set_rx_freq(&rf_device_, 0, config.rx_freq_hz);
  std::cout << "  Actual RX frequency: " << actual_rx_freq / 1e6 << " MHz" << std::endl;
  
  // Set TX frequency
  std::cout << "Setting TX frequency: " << config.tx_freq_hz / 1e6 << " MHz" << std::endl;
  double actual_tx_freq = srsran_rf_set_tx_freq(&rf_device_, 0, config.tx_freq_hz);
  std::cout << "  Actual TX frequency: " << actual_tx_freq / 1e6 << " MHz" << std::endl;
  
  // Set TX gain
  std::cout << "Setting TX gain: " << config.tx_gain_db << " dB" << std::endl;
  if (srsran_rf_set_tx_gain(&rf_device_, config.tx_gain_db) != SRSRAN_SUCCESS) {
      std::cerr << "Error setting TX gain" << std::endl;
      srsran_rf_close(&rf_device_);
      return false;
  }
  
  initialized_ = true;
  std::cout << "RF device initialized successfully" << std::endl;
  
  return true;
}

bool RfHandler::start_rx() {
  if (!initialized_) {
      std::cerr << "RF Handler not initialized" << std::endl;
      return false;
  }
  
  if (srsran_rf_start_rx_stream(&rf_device_, false) != SRSRAN_SUCCESS) {
      std::cerr << "Error starting RX stream" << std::endl;
      return false;
  }
  
  return true;
}

bool RfHandler::stop_rx() {
  if (!initialized_) {
      std::cerr << "RF Handler not initialized" << std::endl;
      return false;
  }
  
  if (srsran_rf_stop_rx_stream(&rf_device_) != SRSRAN_SUCCESS) {
      std::cerr << "Error stopping RX stream" << std::endl;
      return false;
  }
  
  return true;
}

bool RfHandler::start_tx() {
  if (!initialized_) {
      std::cerr << "RF Handler not initialized" << std::endl;
      return false;
  }
  
  // Note: srsRAN doesn't have explicit start_tx_stream
  // TX stream starts automatically on first send
  std::cout << "TX stream ready (will start on first transmission)" << std::endl;
  return true;
}

bool RfHandler::stop_tx() {
  if (!initialized_) {
      std::cerr << "RF Handler not initialized" << std::endl;
      return false;
  }
  
  // Send end-of-burst to flush TX buffer
  std::cout << "Stopping TX stream..." << std::endl;
  return true;
}

int RfHandler::receive(std::complex<float>* buffer, uint32_t nsamples) {
  if (!initialized_) {
      std::cerr << "RF Handler not initialized" << std::endl;
      return -1;
  }
  
  // Use blocking mode (1) for reliable sample reception
  int nrecv = srsran_rf_recv(&rf_device_, buffer, nsamples, 1);
  
  if (nrecv < 0) {
      std::cerr << "[ERROR] Receive failed with error code: " << nrecv << std::endl;
      return nrecv;
  }
  
  return nrecv;
}

int RfHandler::transmit(const std::complex<float>* buffer, uint32_t nsamples,
                  bool start_of_burst, bool end_of_burst) {
  if (!initialized_) {
      std::cerr << "[RF ERROR] RF Handler not initialized" << std::endl;
      return -1;
  }
  
  // For non-const buffer
  void* buffers[1] = {const_cast<std::complex<float>*>(buffer)};
  
  // Using srsran_rf_send_multi for non-timed transmission
  int nsent = srsran_rf_send_multi(&rf_device_, buffers, nsamples, 
                               true, start_of_burst, end_of_burst);
  
  if (nsent < 0) {
      // Only print error occasionally to avoid log spam
      static int error_count = 0;
      if (error_count++ % 100 == 0) {
          std::cerr << "\n[RF] Transmission error (count: " << error_count << ")" << std::endl;
      }
      return nsent;
  }
  
  // Warn if we didn't transmit all samples (only first few times)
  if (nsent != (int)nsamples) {
      static int warn_count = 0;
      if (warn_count++ < 5) {
          std::cerr << "\n[RF] Transmitted " << nsent << " samples, expected " << nsamples << std::endl;
      }
  }
  
  return nsent;
}

double RfHandler::set_rx_freq(double freq_hz) {
  if (!initialized_) {
      std::cerr << "RF Handler not initialized" << std::endl;
      return 0.0;
  }
  
  return srsran_rf_set_rx_freq(&rf_device_, 0, freq_hz);
}

double RfHandler::set_tx_freq(double freq_hz) {
  if (!initialized_) {
      std::cerr << "RF Handler not initialized" << std::endl;
      return 0.0;
  }
  
  return srsran_rf_set_tx_freq(&rf_device_, 0, freq_hz);
}

bool RfHandler::set_rx_gain(double gain_db) {
  if (!initialized_) {
      return false;
  }
  
  return srsran_rf_set_rx_gain(&rf_device_, gain_db) == SRSRAN_SUCCESS;
}

bool RfHandler::set_tx_gain(double gain_db) {
  if (!initialized_) {
      return false;
  }
  
  return srsran_rf_set_tx_gain(&rf_device_, gain_db) == SRSRAN_SUCCESS;
}

double RfHandler::set_sample_rate(double srate_hz) {
  if (!initialized_) {
      std::cerr << "RF Handler not initialized" << std::endl;
      return 0.0;
  }
  
  double actual_srate_rx = srsran_rf_set_rx_srate(&rf_device_, srate_hz);
  double actual_srate_tx = srsran_rf_set_tx_srate(&rf_device_, srate_hz);
  
  return actual_srate_rx;
}

void RfHandler::get_time(time_t& full_secs, double& frac_secs) {
  if (!initialized_) {
      full_secs = 0;
      frac_secs = 0.0;
      return;
  }
  
  srsran_rf_get_time(&rf_device_, &full_secs, &frac_secs);
}

} // namespace ssb_spoofer
