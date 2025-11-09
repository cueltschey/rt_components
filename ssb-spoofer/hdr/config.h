#ifndef SSB_SPOOFER_CONFIG_H
#define SSB_SPOOFER_CONFIG_H

#include <string> 
#include <cstdint>

namespace ssb_spoofer {

/**
* RF configuration parameters
*/
struct RfConfig {
  std::string device_name;
  std::string device_args;
  double      tx_freq_hz;
  double      rx_freq_hz;
  double      srate_hz;
  double      tx_gain_db;
  double      rx_gain_db;
};

/**
* SSB configuration parameters
*/
struct SsbConfig {
  std::string pattern;
  uint32_t    scs_khz; 
  uint32_t    periodicity_ms;   ///< SSB periodicity in ms
  double      f_offset_hz;
  double      ssb_freq_offset_hz; ///< SSB frequency offset
  float       beta_pss;         ///< PSS power allocation
  float       beta_sss;         ///< SSS power allocation
  float       beta_pbch;        ///< PBCH power allocation
  float       beta_pbch_dmrs;   ///< PBCH DMRS power allocation 
  
};

/** 
* Attack configuration parameters
*/
struct AttackConfig {
  uint32_t  target_pci;
  bool      scan_for_target;

  // MIB Modification flags
  bool      modify_coreset0_idx;
  bool      modify_ss0_idx;
  bool      modify_cell_barred;
  bool      modify_intra_freq_resel;

  uint32_t  coreset0_idx_value;
  uint32_t  ss0_idx_value;
  bool      cell_barred_value;
  bool      intra_freq_resel_value;

  // Transmission Parameters
  double    tx_power_db;
  double    tx_power_offset_db;
  bool      continuous_tx;
  
  // Burst Control Parameters
  uint64_t  max_bursts;              ///< Maximum number of bursts (0 = unlimited)
  uint32_t  burst_interval_us;       ///< Delay between bursts in microseconds (0 = minimum delay)
  uint32_t  burst_length_ms;         ///< Length of each burst in milliseconds (controls samples per burst)
};

/**
* Operational parameters
*/
struct OperationalConfig {
  double scan_duration_sec;
  std::string log_level;
  std::string log_file;
  bool save_samples;
  std::string samples_file;

};

/**
* Complete configuration structure
*/
struct Config {
  RfConfig rf;
  SsbConfig ssb;
  AttackConfig attack;
  OperationalConfig operation;
};

/**
* Configuration parser
*/
class ConfigParser {
public:
  /**
  * Load confifuration from the file
  * @param filename Path to YAML configuration file
  * @param config Output configuration structure
  * @return true if successful, false otherwise
  */
  static bool load_from_file(const std::string& filename, Config& config);

  /**
  * Validate configuration parameters
  * @param config Configuration to validate
  * @return true if successful, false otherwise
  */
  static bool validate(const Config& config);
  
  /**
  * print configuration to console
  * @param config Configuration to print 
  */
  static void print(const Config& config);

};

} // namespace ssb_spoofer

#endif
