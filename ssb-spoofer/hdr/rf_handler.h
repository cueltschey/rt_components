#ifndef SSB_SPOOFER_RF_HANDLER_H
#define SSB_SPOOFER_RF_HANDLER_H

#include "config.h"
#include "srsran/phy/rf/rf.h"
#include <complex>
#include <vector> 

namespace ssb_spoofer {

class RfHandler {
public:
  RfHandler();
  ~RfHandler();

  /**
  * @brief Initialize rf configuration
  * @param config RF configuration
  * @return true if successful, false otherwise
  */
  bool init(const RfConfig& config);

  /**
  * @brief Start rx stream
  * @return true if successful, false otherwise 
  */
  bool start_rx();

  /**
  * @brief Stop rx stream
  * @return true if successful, false oterwise
  */
  bool stop_rx();

  /**
  * @brief Start tx stream
  * @return true if successful, false otherwise
  */
  bool start_tx();

  /**
  * @brief Stop tx stream
  * @return true if successful, false otherwise
  */
  bool stop_tx();

  /**
  * @brief Receive sampels from RF device
  * @param buffer Buffer to store received samples
  * @param nsamples Number of samples to receive
  * @return Number of samples actually received, negetive on error
  */
  int receive(std::complex<float>* buffer, uint32_t nsamples);

  /**
  * @brief Transmit samples from RF device
  * @param buffer Buffer to store the samples to be transmitted
  * @param nsamples Number of samples to transmit
  * @param start_of_burst true if it the start of burst, false otherwise
  * @param end_of_burst true if it is the end of burst, false otherwise
  * @return Number of samples actually transmitted, negetive on error
  */
  int transmit(const std::complex<float>* buffer, uint32_t nsamples,
               bool start_of_burst = false, bool end_of_burst = false);

  /**
  * mutators
  */
  double set_tx_freq(double freq_hz);
  double set_rx_freq(double freq_hz);
  bool set_tx_gain(double gain_db);
  bool set_rx_gain(double gain_db);
  double set_sample_rate(double srate_hz);
  void get_time(time_t& full_secs, double& frac_secs);

  /**
  * Check if RG device is initialized
  * @return true if initialized, false otherwise
  */
  bool is_initialized() const { return initialized_; }


private:
  srsran_rf_t rf_device_;
  bool initialized_;
  RfConfig config_;

  // Disable copy
  RfHandler(const RfHandler&) = delete;
  RfHandler& operator = (const RfHandler&) = delete;
};
} // namespace ssb_spoofer

#endif
