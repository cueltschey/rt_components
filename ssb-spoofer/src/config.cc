// Config parser - dead simple YAML-like parser for our config files

#include "config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

namespace ssb_spoofer {

// parses basic YAML-style config file
static std::map<std::string, std::string> parse_config_file(const std::string& filename) {
  std::map<std::string, std::string> config_map;
  std::ifstream file(filename);
  
  if (!file.is_open()) {
      std::cerr << "[!] couldn't open config: " << filename << "\n";
      return config_map;
  }
  
  std::string line;
  std::string current_section;
  
  while (std::getline(file, line)) {
      // strip comments
      size_t comment_pos = line.find('#');
      if (comment_pos != std::string::npos) {
      line = line.substr(0, comment_pos);
      }
      
      // trim whitespace
      size_t start = line.find_first_not_of(" \t");
      if (start == std::string::npos) continue;
      size_t end = line.find_last_not_of(" \t");
      line = line.substr(start, end - start + 1);
      
      if (line.empty()) continue;
      
      // section headers end with :
      if (line.back() == ':' && line.find(':') == line.length() - 1) {
      current_section = line.substr(0, line.length() - 1);
      continue;
      }
      
      // parse key: value
      size_t colon_pos = line.find(':');
      if (colon_pos != std::string::npos) {
      std::string key = line.substr(0, colon_pos);
      std::string value = line.substr(colon_pos + 1);
      
      // trim
      key.erase(0, key.find_first_not_of(" \t"));
      key.erase(key.find_last_not_of(" \t") + 1);
      value.erase(0, value.find_first_not_of(" \t"));
      value.erase(value.find_last_not_of(" \t") + 1);
      
      // remove quotes
      if (value.front() == '"' && value.back() == '"') {
          value = value.substr(1, value.length() - 2);
      }
      
      std::string full_key = current_section.empty() ? key : current_section + "." + key;
      config_map[full_key] = value;
      }
  }
  
  return config_map;
}

template<typename T>
static T get_value(const std::map<std::string, std::string>& config_map,
             const std::string& key, const T& default_value) {
  auto it = config_map.find(key);
  if (it == config_map.end()) {
      return default_value;
  }
  
  std::istringstream iss(it->second);
  T value;
  if (!(iss >> value)) {
      return default_value;
  }
  return value;
}

template<>
std::string get_value<std::string>(const std::map<std::string, std::string>& config_map,
                              const std::string& key, const std::string& default_value) {
  auto it = config_map.find(key);
  return (it != config_map.end()) ? it->second : default_value;
}

template<>
bool get_value<bool>(const std::map<std::string, std::string>& config_map,
               const std::string& key, const bool& default_value) {
  auto it = config_map.find(key);
  if (it == config_map.end()) {
      return default_value;
  }
  
  std::string value = it->second;
  return (value == "true" || value == "True" || value == "TRUE" || value == "1");
}

bool ConfigParser::load_from_file(const std::string& filename, Config& config) {
  auto config_map = parse_config_file(filename);
  
  if (config_map.empty()) {
      std::cerr << "[!] config file empty or parse failed\n";
      return false;
  }
  
  // RF config
  config.rf.device_name                 = get_value<std::string>(config_map, "rf.device_name", "uhd");
  config.rf.device_args                 = get_value<std::string>(config_map, "rf.device_args", "");
  config.rf.rx_freq_hz                  = get_value<double>(config_map, "rf.rx_freq_hz", 3510000000.0);
  config.rf.tx_freq_hz                  = get_value<double>(config_map, "rf.tx_freq_hz", 3510000000.0);
  config.rf.srate_hz                    = get_value<double>(config_map, "rf.sample_rate_hz", 23040000.0);
  config.rf.rx_gain_db                  = get_value<double>(config_map, "rf.rx_gain_db", 40.0);
  config.rf.tx_gain_db                  = get_value<double>(config_map, "rf.tx_gain_db", 60.0);
  
  // SSB config
  config.ssb.pattern                    = get_value<std::string>(config_map, "ssb.pattern", "C");
  config.ssb.scs_khz                    = get_value<uint32_t>(config_map, "ssb.scs_khz", 30);
  config.ssb.periodicity_ms             = get_value<uint32_t>(config_map, "ssb.periodicity_ms", 20);
  config.ssb.ssb_freq_offset_hz         = get_value<double>(config_map, "ssb.ssb_freq_offset_hz", 0.0);
  config.ssb.beta_pss                   = get_value<float>(config_map, "ssb.beta_pss", 0.0f);
  config.ssb.beta_sss                   = get_value<float>(config_map, "ssb.beta_sss", 0.0f);
  config.ssb.beta_pbch                  = get_value<float>(config_map, "ssb.beta_pbch", 0.0f);
  config.ssb.beta_pbch_dmrs             = get_value<float>(config_map, "ssb.beta_pbch_dmrs", 0.0f);
  
  // attack config
  config.attack.target_pci              = get_value<uint32_t>(config_map, "attack.target_pci", 0);
  config.attack.scan_for_target         = get_value<bool>(config_map, "attack.scan_for_target", true);
  config.attack.modify_cell_barred      = get_value<bool>(config_map, "attack.modify_cell_barred", true);
  config.attack.cell_barred_value       = get_value<bool>(config_map, "attack.cell_barred_value", true);
  config.attack.modify_coreset0_idx     = get_value<bool>(config_map, "attack.modify_coreset0_idx", false);
  config.attack.coreset0_idx_value      = get_value<uint32_t>(config_map, "attack.coreset0_idx_value", 15);
  config.attack.modify_ss0_idx          = get_value<bool>(config_map, "attack.modify_ss0_idx", false);
  config.attack.ss0_idx_value           = get_value<uint32_t>(config_map, "attack.ss0_idx_value", 15);
  config.attack.modify_intra_freq_resel = get_value<bool>(config_map, "attack.modify_intra_freq_resel", false);
  config.attack.intra_freq_resel_value  = get_value<bool>(config_map, "attack.intra_freq_resel_value", false);
  config.attack.tx_power_offset_db      = get_value<double>(config_map, "attack.tx_power_offset_db", 0.0);
  config.attack.continuous_tx           = get_value<bool>(config_map, "attack.continuous_tx", true);
  
  // Parse burst control parameters
  config.attack.max_bursts              = get_value<uint64_t>(config_map, "attack.max_bursts", 0);
  config.attack.burst_interval_us       = get_value<uint32_t>(config_map, "attack.burst_interval_us", 500);
  config.attack.burst_length_ms         = get_value<uint32_t>(config_map, "attack.burst_length_ms", 1);
  
  // operation config
  config.operation.scan_duration_sec    = get_value<double>(config_map, "operation.scan_duration_sec", 10.0);
  config.operation.log_level            = get_value<std::string>(config_map, "operation.log_level", "info");
  config.operation.log_file             = get_value<std::string>(config_map, "operation.log_file", "ssb_spoofer.log");
  config.operation.save_samples         = get_value<bool>(config_map, "operation.save_samples", false);
  config.operation.samples_file         = get_value<std::string>(config_map, "operation.samples_file", "rx_samples.dat");
  
  return validate(config);
}

bool ConfigParser::validate(const Config& config) {
  bool valid = true;
  
  if (config.rf.srate_hz <= 0) {
      std::cerr << "[!] invalid sample rate\n";
      valid = false;
  }
  
  if (config.rf.rx_freq_hz <= 0 || config.rf.tx_freq_hz <= 0) {
      std::cerr << "[!] invalid frequency\n";
      valid = false;
  }
  
  if (config.ssb.pattern != "A" && config.ssb.pattern != "B" && 
      config.ssb.pattern != "C" && config.ssb.pattern != "D" && 
      config.ssb.pattern != "E") {
      std::cerr << "[!] invalid SSB pattern (need A/B/C/D/E)\n";
      valid = false;
  }
  
  if (config.ssb.scs_khz != 15 && config.ssb.scs_khz != 30) {
      std::cerr << "[!] invalid SCS (need 15 or 30 kHz)\n";
      valid = false;
  }
  
  if (config.attack.target_pci > 1007) {
      std::cerr << "[!] invalid PCI (max 1007)\n";
      valid = false;
  }
  
  if (config.attack.coreset0_idx_value > 15) {
      std::cerr << "[!] invalid CORESET0 idx (max 15)\n";
      valid = false;
  }
  
  if (config.attack.ss0_idx_value > 15) {
      std::cerr << "[!] invalid SS0 idx (max 15)\n";
      valid = false;
  }
  
  return valid;
}

void ConfigParser::print(const Config& config) {
  std::cout << "\n--- Configuration ---\n";
  std::cout << "\n[RF]\n";
  std::cout << "  device: " << config.rf.device_name << "\n";
  std::cout << "  args: " << config.rf.device_args << "\n";
  std::cout << "  RX freq: " << config.rf.rx_freq_hz / 1e6 << " MHz\n";
  std::cout << "  TX freq: " << config.rf.tx_freq_hz / 1e6 << " MHz\n";
  std::cout << "  srate: " << config.rf.srate_hz / 1e6 << " MHz\n";
  std::cout << "  RX gain: " << config.rf.rx_gain_db << " dB\n";
  std::cout << "  TX gain: " << config.rf.tx_gain_db << " dB\n";
  
  std::cout << "\n[SSB]\n";
  std::cout << "  pattern: " << config.ssb.pattern << "\n";
  std::cout << "  SCS: " << config.ssb.scs_khz << " kHz\n";
  std::cout << "  period: " << config.ssb.periodicity_ms << " ms\n";
  
  std::cout << "\n[Attack]\n";
  std::cout << "  target PCI: " << config.attack.target_pci << "\n";
  std::cout << "  scan for target: " << (config.attack.scan_for_target ? "yes" : "no") << "\n";
  std::cout << "  modify cell_barred: " << (config.attack.modify_cell_barred ? "yes" : "no");
  if (config.attack.modify_cell_barred) {
      std::cout << " (" << (config.attack.cell_barred_value ? "true" : "false") << ")";
  }
  std::cout << "\n";
  std::cout << "  modify CORESET0: " << (config.attack.modify_coreset0_idx ? "yes" : "no");
  if (config.attack.modify_coreset0_idx) {
      std::cout << " (val: " << config.attack.coreset0_idx_value << ")";
  }
  std::cout << "\n";
  std::cout << "  continuous TX: " << (config.attack.continuous_tx ? "yes" : "no") << "\n";
  
  std::cout << "\n[Burst Control]\n";
  std::cout << "  max bursts: " << (config.attack.max_bursts == 0 ? "unlimited" : std::to_string(config.attack.max_bursts)) << "\n";
  std::cout << "  burst interval: " << config.attack.burst_interval_us << " us\n";
  std::cout << "  burst length: " << config.attack.burst_length_ms << " ms\n";
  
  std::cout << "\n--------------------\n\n";
}

} // namespace ssb_spoofer

