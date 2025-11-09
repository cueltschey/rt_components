#include "rf_uhd.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <algorithm>

// Collect IQ data using UHD
uuagent_error_e RF_UHD::collect_iq_data(const all_args_t &args) {
  // Create a USRP device
  try {
    uhd::usrp::multi_usrp::sptr usrp =
        uhd::usrp::multi_usrp::make(args.rf.device_args);

    // Configure the USRP
    usrp->set_rx_rate(args.rf.srate_hz);
    usrp->set_rx_freq(uhd::tune_request_t(args.rf.rx_freq));
    usrp->set_rx_gain(args.rf.rx_gain);

    // Set up the RX streamer
    uhd::stream_args_t stream_args("fc32",
                                   "sc16"); // Floating-point complex format
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // Buffer for IQ data
    const size_t samps_per_buff = rx_stream->get_max_num_samps();
    std::vector<std::complex<float>> buffer(samps_per_buff);
    uhd::rx_metadata_t md;

    std::ofstream outfile(args.rf.output_file, std::ios::binary);
    if (!outfile) {
      std::cerr << "failed to open output file: " << args.rf.output_file
                << std::endl;
      return UUAGENT_FILE_ERROR;
    }
    // Start streaming
    std::cout << "Receiving " << args.rf.num_samples << " samples...\n";
    uhd::stream_cmd_t stream_cmd(
        uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = args.rf.num_samples;
    stream_cmd.stream_now = true;
    rx_stream->issue_stream_cmd(stream_cmd);

    // Loop until we receive all the samples needed
    size_t total_received = 0;
    while (total_received < args.rf.num_samples) {
      size_t num_to_recv = std::min(samps_per_buff,
                                   args.rf.num_samples - total_received);
      size_t n = rx_stream->recv(buffer.data(), num_to_recv, md, 3.0);

      if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
        std::cerr << "Error receiving samples: " << md.strerror() << std::endl;
        return UUAGENT_SAMPLE_ERROR;
      }

      outfile.write(reinterpret_cast<const char *>(buffer.data()),
                    n * sizeof(std::complex<float>));
      total_received += n;
    }

    outfile.close();

    std::cout << "IQ Data saved to " << args.rf.output_file << std::endl;
    return UUAGENT_SUCCESS;
  } catch (const std::exception& e) {
    std::cerr << "UHD Error: " << e.what() << std::endl;
    return UUAGENT_UHD_ERROR;
  }
}
