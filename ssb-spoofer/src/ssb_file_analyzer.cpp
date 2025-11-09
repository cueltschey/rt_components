// SSB File Analyzer - processes IQ sample files to find and decode SSBs
// useful for debugging without RF hardware

#include "config.h"
#include "ssb_processor.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <cstring>
#include <iomanip>
#include <map>

using namespace ssb_spoofer;

struct AnalyzerArgs {
    std::string input_file;
    double sample_rate_hz = 23.04e6;
    double center_freq_hz = 1842.5e6;
    std::string ssb_pattern = "A";
    uint32_t scs_khz = 15;
    uint32_t periodicity_ms = 20;
    double ssb_freq_offset_hz = 0.0;
    uint32_t target_pci = 0;
    bool scan_for_target = false;
    uint32_t max_samples = 0;  // 0 = all samples
    uint32_t window_size_ms = 10;  // Search window size in ms
    bool verbose = false;
};

void print_usage(const char* program) {
    std::cout << "SSB File Analyzer - decode SSBs from IQ files\n\n";
    std::cout << "usage: " << program << " -f <file> -s <srate> -c <freq> [options]\n\n";
    std::cout << "required:\n";
    std::cout << "  -f, --file <path>       input file (complex float32)\n";
    std::cout << "  -s, --srate <Hz>        sample rate (e.g. 23.04e6)\n";
    std::cout << "  -c, --center-freq <Hz>  center frequency (e.g. 1842.5e6)\n\n";
    std::cout << "optional:\n";
    std::cout << "  -p, --pattern <A-E>     SSB pattern (default: A)\n";
    std::cout << "  --scs <kHz>             subcarrier spacing (default: 15)\n";
    std::cout << "  --period <ms>           SSB periodicity (default: 20)\n";
    std::cout << "  --offset <Hz>           SSB freq offset (default: 0)\n";
    std::cout << "  --pci <id>              target PCI (default: any)\n";
    std::cout << "  --max-samples <N>       max samples to process\n";
    std::cout << "  --window <ms>           search window size (default: 10)\n";
    std::cout << "  -v, --verbose           verbose output\n";
    std::cout << "  -h, --help              show this\n\n";
    std::cout << "examples:\n";
    std::cout << "  " << program << " -f rx_samples.dat -s 23.04e6 -c 1842.5e6\n";
    std::cout << "  " << program << " -f samples.fc32 -s 23.04e6 -c 2.6e9 --pci 500\n";
}

bool parse_args(int argc, char** argv, AnalyzerArgs& args) {
    if (argc < 2) {
        return false;
    }
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            return false;
        } else if (arg == "-f" || arg == "--file") {
            if (++i < argc) args.input_file = argv[i];
        } else if (arg == "-s" || arg == "--srate") {
            if (++i < argc) args.sample_rate_hz = std::stod(argv[i]);
        } else if (arg == "-c" || arg == "--center-freq") {
            if (++i < argc) args.center_freq_hz = std::stod(argv[i]);
        } else if (arg == "-p" || arg == "--pattern") {
            if (++i < argc) args.ssb_pattern = argv[i];
        } else if (arg == "--scs") {
            if (++i < argc) args.scs_khz = std::stoul(argv[i]);
        } else if (arg == "--period") {
            if (++i < argc) args.periodicity_ms = std::stoul(argv[i]);
        } else if (arg == "--offset") {
            if (++i < argc) args.ssb_freq_offset_hz = std::stod(argv[i]);
        } else if (arg == "--pci") {
            if (++i < argc) {
                args.target_pci = std::stoul(argv[i]);
                args.scan_for_target = true;
            }
        } else if (arg == "--max-samples") {
            if (++i < argc) args.max_samples = std::stoul(argv[i]);
        } else if (arg == "--window") {
            if (++i < argc) args.window_size_ms = std::stoul(argv[i]);
        } else if (arg == "-v" || arg == "--verbose") {
            args.verbose = true;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            return false;
        }
    }
    
    // validate
    if (args.input_file.empty()) {
        std::cerr << "error: input file required (-f)\n";
        return false;
    }
    if (args.sample_rate_hz <= 0) {
        std::cerr << "error: sample rate required (-s)\n";
        return false;
    }
    if (args.center_freq_hz <= 0) {
        std::cerr << "error: center freq required (-c)\n";
        return false;
    }
    
    return true;
}

bool load_samples(const std::string& filename, std::vector<std::complex<float>>& samples, 
                 uint32_t max_samples = 0) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "error: can't open " << filename << "\n";
        return false;
    }
    
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    size_t num_samples = file_size / sizeof(std::complex<float>);
    
    if (max_samples > 0 && max_samples < num_samples) {
        num_samples = max_samples;
    }
    
    samples.resize(num_samples);
    file.read(reinterpret_cast<char*>(samples.data()), 
              num_samples * sizeof(std::complex<float>));
    
    if (!file) {
        std::cerr << "error reading file\n";
        return false;
    }
    
    return true;
}

void print_sample_stats(const std::vector<std::complex<float>>& samples, double srate_hz) {
    if (samples.empty()) return;
    
    float max_mag = 0.0f;
    float sum_power = 0.0f;
    
    for (const auto& s : samples) {
        float mag = std::abs(s);
        max_mag = std::max(max_mag, mag);
        sum_power += std::norm(s);
    }
    
    float avg_power = sum_power / samples.size();
    float avg_power_db = 10.0f * std::log10(avg_power + 1e-12f);
    
    std::cout << "\n--- Sample Stats ---\n";
    std::cout << "  samples: " << samples.size() << "\n";
    std::cout << "  duration: " << std::fixed << std::setprecision(3) 
              << (samples.size() / srate_hz * 1000.0) << " ms\n";
    std::cout << "  max mag: " << std::fixed << std::setprecision(4) << max_mag << "\n";
    std::cout << "  avg power: " << std::fixed << std::setprecision(2) 
              << avg_power_db << " dB\n";
    std::cout << "--------------------\n\n";
}

int main(int argc, char** argv) {
    AnalyzerArgs args;
    
    if (!parse_args(argc, argv, args)) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::cout << "\nSSB File Analyzer\n\n";
    
    // show config
    std::cout << "config:\n";
    std::cout << "  file: " << args.input_file << "\n";
    std::cout << "  srate: " << args.sample_rate_hz / 1e6 << " MHz\n";
    std::cout << "  center: " << args.center_freq_hz / 1e6 << " MHz\n";
    std::cout << "  pattern: " << args.ssb_pattern << "\n";
    std::cout << "  SCS: " << args.scs_khz << " kHz\n";
    std::cout << "  period: " << args.periodicity_ms << " ms\n";
    if (args.ssb_freq_offset_hz != 0.0) {
        std::cout << "  offset: " << args.ssb_freq_offset_hz / 1e6 << " MHz\n";
    }
    if (args.scan_for_target) {
        std::cout << "  target PCI: " << args.target_pci << "\n";
    }
    std::cout << "\n";
    
    // load samples
    std::cout << "loading samples...\n";
    std::vector<std::complex<float>> samples;
    if (!load_samples(args.input_file, samples, args.max_samples)) {
        return 1;
    }
    
    print_sample_stats(samples, args.sample_rate_hz);
    
    // init SSB processor
    std::cout << "initializing SSB processor...\n";
    SsbConfig ssb_config;
    ssb_config.pattern = args.ssb_pattern;
    ssb_config.scs_khz = args.scs_khz;
    ssb_config.periodicity_ms = args.periodicity_ms;
    ssb_config.ssb_freq_offset_hz = args.ssb_freq_offset_hz;
    ssb_config.beta_pss = 0.0f;
    ssb_config.beta_sss = 0.0f;
    ssb_config.beta_pbch = 0.0f;
    ssb_config.beta_pbch_dmrs = 0.0f;
    
    SsbProcessor ssb_proc;
    if (!ssb_proc.init(ssb_config, args.sample_rate_hz, args.center_freq_hz)) {
        std::cerr << "SSB processor init failed\n";
        return 1;
    }
    
    // calc search window
    uint32_t window_samples = static_cast<uint32_t>(
        args.sample_rate_hz * args.window_size_ms / 1000.0);
    
    std::cout << "\n--- Scanning ---\n";
    std::cout << "window: " << args.window_size_ms << " ms (" 
              << window_samples << " samples)\n\n";
    
    // search through file in windows (50% overlap)
    uint32_t window_count = 0;
    uint32_t ssb_count = 0;
    std::vector<SsbSearchResult> found_ssbs;
    
    for (uint32_t offset = 0; offset + window_samples <= samples.size(); 
         offset += window_samples / 2) {
        
        window_count++;
        
        std::optional<uint32_t> target_pci = std::nullopt;
        if (args.scan_for_target) {
            target_pci = args.target_pci;
        }
        
        SsbSearchResult result = ssb_proc.scan(
            &samples[offset], window_samples, target_pci);
        
        if (result.found) {
            ssb_count++;
            found_ssbs.push_back(result);
            
            double time_ms = offset / args.sample_rate_hz * 1000.0;
            
            std::cout << "\n[+] SSB #" << ssb_count << " at " 
                      << std::fixed << std::setprecision(2) << time_ms << " ms\n";
            std::cout << "    PCI: " << result.pci << "\n";
            std::cout << "    SSB idx: " << result.ssb_idx << "\n";
            std::cout << "    SNR: " << std::fixed << std::setprecision(1) 
                      << result.snr_db << " dB\n";
            std::cout << "    RSRP: " << std::fixed << std::setprecision(1) 
                      << result.rsrp_dbm << " dBm\n";
            
            if (args.verbose) {
                SsbProcessor::print_mib(result.mib);
            }
        }
        
        // progress
        if (window_count % 10 == 0 && !args.verbose) {
            std::cout << "." << std::flush;
        }
    }
    
    // summary
    std::cout << "\n\n--- Results ---\n";
    std::cout << "  windows: " << window_count << "\n";
    std::cout << "  SSBs found: " << ssb_count << "\n";
    
    if (!found_ssbs.empty()) {
        std::cout << "\n--- SSB Summary ---\n";
        
        // group by PCI
        std::map<uint32_t, std::vector<SsbSearchResult>> by_pci;
        for (const auto& ssb : found_ssbs) {
            by_pci[ssb.pci].push_back(ssb);
        }
        
        for (const auto& [pci, ssbs] : by_pci) {
            float avg_snr = 0.0f;
            float avg_rsrp = 0.0f;
            for (const auto& ssb : ssbs) {
                avg_snr += ssb.snr_db;
                avg_rsrp += ssb.rsrp_dbm;
            }
            avg_snr /= ssbs.size();
            avg_rsrp /= ssbs.size();
            
            std::cout << "\nPCI " << pci << ":\n";
            std::cout << "  count: " << ssbs.size() << "\n";
            std::cout << "  avg SNR: " << std::fixed << std::setprecision(1) 
                      << avg_snr << " dB\n";
            std::cout << "  avg RSRP: " << std::fixed << std::setprecision(1) 
                      << avg_rsrp << " dBm\n";
            std::cout << "  SSB idx: " << ssbs[0].ssb_idx << "\n";
            
            if (args.verbose) {
                std::cout << "\n  MIB:\n";
                SsbProcessor::print_mib(ssbs[0].mib);
            }
        }
    } else {
        std::cout << "\n[!] no SSBs found\n";
        std::cout << "\ntroubleshooting:\n";
        std::cout << "  - check sample rate and frequency\n";
        std::cout << "  - try different SSB pattern\n";
        std::cout << "  - need at least 10ms of samples\n";
        std::cout << "  - check signal strength\n";
    }
    
    std::cout << "\n";
    return 0;
}

