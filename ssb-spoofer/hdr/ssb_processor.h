#ifndef SSB_SPOOFER_SSB_PROCESSOR_H
#define SSB_SPOOFER_SSB_PROCESSOR_H

#include "config.h"
#include "srsran/phy/common/phy_common.h"
#include "srsran/common/common.h"
#include "srsran/srsran.h"
#include <complex>
#include <vector>
#include <optional>
#include <cstdint>


namespace ssb_spoofer {

/**
* * SSB search result
*/
struct SsbSearchResult {
  bool found;
  uint32_t pci;                   // Physical Cell ID
  uint32_t ssb_idx;               // SSB index
  srsran_mib_nr_t mib;            // Decoded MIB
  srsran_pbch_msg_nr_t pbch_msg;  // Raw PBCH message
  float snr_db;                   // Signal-to-noise ratio
  float rsrp_dbm;                 // Reference signal received power
};

/**
* SSB Processor for scanning, decofing, modifying, and encoding SSBs
*/
class SsbProcessor {
public:
  SsbProcessor();
  ~SsbProcessor();

  bool init(const SsbConfig& config, double sample_rate_hz, double center_freq_hz);

  bool configure(const SsbConfig& config, double srate_hz, double center_freq_hz);

  SsbSearchResult scan(const std::complex<float>* buffer, uint32_t nsamples,
                 std::optional<uint32_t> target_pci = std::nullopt);
  
  bool decode_mib(const srsran_pbch_msg_nr_t& pbch_msg, srsran_mib_nr_t& mib);

  bool modify_mib(srsran_mib_nr_t& mib, const AttackConfig& attack_config);

  bool encode_mib(const srsran_mib_nr_t& mib, uint32_t ssb_idx,
            bool hrf, srsran_pbch_msg_nr_t& pbch_msg);
  
  uint32_t generate_ssb(uint32_t pci, const srsran_pbch_msg_nr_t& pbch_msg,
                  std::complex<float>* output, uint32_t ssb_idx = 0);

  // MULTI-PCI BURST MODE - Generate hundreds of SSBs with different PCIs rapidly
  uint32_t generate_multi_pci_burst(const srsran_pbch_msg_nr_t& pbch_msg,
                                     std::complex<float>* output,
                                     uint32_t start_pci,
                                     uint32_t num_pcis,
                                     uint32_t ssb_idx = 0);

  uint32_t get_ssb_size() const;
  uint32_t get_subframe_size() const;
  static void print_mib(const srsran_mib_nr_t& mib);
  bool is_initialized() const { return initialized_; }
  
private:
  srsran_ssb_t ssb_;
  bool initialized_;
  SsbConfig config_;
  double srate_hz_;
  double center_freq_hz_;

  srsran_ssb_pattern_t pattern_from_string(const std::string& pattern);
  srsran_subcarrier_spacing_t scs_from_khz(uint32_t scs_khz);

  // Disable copy
  SsbProcessor(const SsbProcessor&) = delete;
  SsbProcessor& operator = (const SsbProcessor&) = delete;
};

} // namespace ssb_spoofer


#endif  // SSB_SPOOFER_SSB_PROCESSOR_HJK
