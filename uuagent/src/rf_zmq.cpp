#include "rf_zmq.h"
#include <iostream>
#include <fstream>
#include <complex>

RF_ZMQ::RF_ZMQ() : context(1), socket(context, ZMQ_SUB) {
  // Constructor
}

RF_ZMQ::~RF_ZMQ() {
  // Cleanup handled by zmq objects
}

uuagent_error_e RF_ZMQ::collect_iq_data(const all_args_t& args) {
  try {
    // Lets connect to the ZMQ publisher
    std::string address = args.rf.device_args.empty() ? "tcp://localhost:5555" : args.rf.device_args;
    socket.connect(address);
    socket.set(zmq::sockopt::subscribe, "");

    // Lets set a timeout of 5 secs
    socket.set(zmq::sockopt::rcvtimeo, 5000);
    std::cout << "ZMQ: Attempting to connect to the publisher at " << address << "..." << std::endl;

    // Lets now create a buffer to keep the incoming IQ num_samples
    std::vector<std::complex<float>> iq_buffer;
    iq_buffer.reserve(args.rf.num_samples);
    size_t samples_collected = 0;
    bool is_connected = false;  // Just to track if we've received the first message

    // Now, lets loop to collect the required number of samples
    while (samples_collected < args.rf.num_samples) {
      zmq::message_t message;

      auto recv_result = socket.recv(message); // Evaluates to true with non-empty message reception success
      if (recv_result) {
        // Lets first print the successful connection
        if (!is_connected) {
          std::cout << "ZMQ: Connection successful with " << address
                    <<"\nReceiving " << args.rf.num_samples <<" samples...\n(Patiencee...)"
                    << std::endl;
          is_connected = true;
        }

        size_t samples_in_msg = message.size() / sizeof(std::complex<float>);
        if (samples_in_msg == 0) {
          continue; // Ignoring empty packets/messages
        }

        // Lets get a typed pointer to the message's raw data
        const auto* data_ptr = static_cast<const std::complex<float>*>(message.data());

        // Append the samples from this message to our main buffeer
        iq_buffer.insert(iq_buffer.end(), data_ptr, data_ptr + samples_in_msg);

        samples_collected += samples_in_msg;

      } else {
          if (!is_connected) {
              std::cerr << "ZMQ Error: Timed out waiting for the first message. Is the publisher running?"
              << std::endl;
          } else {
              std::cerr << "ZMQ Error: Timed out waiting for subsequent message." << std::endl;
          }
          return UUAGENT_ZMQ_ERROR;
      }
    }

    // Finally, lets save the IQ from our buffer to the file
    std::ofstream outfile(args.rf.output_file, std::ios::binary);
    if (!outfile) {
      std::cerr << "Failed to open output file: " << args.rf.output_file << std::endl;
      return UUAGENT_FILE_ERROR;
    }

    // Lets write exactly the number of samples that is requested
    outfile.write(reinterpret_cast<const char*>(iq_buffer.data()),
                  args.rf.num_samples * sizeof(std::complex<float>));
    outfile.close();

    std::cout << "ZMQ: Saved " << args.rf.num_samples << " samples to "
              << args.rf.output_file << std::endl;
    
    return UUAGENT_SUCCESS;

  } catch (const zmq::error_t& e) {
    // ZMQ specific exceptions
    std::cerr << "ZMQ Error: " << e.what() << "(Error code: " << e.num() << ")" << std::endl;
    return UUAGENT_ZMQ_ERROR;
  } catch (const std::exception& e) {
    // Other standard exceptions
    std::cerr << "ZMQ Error: " << e.what() << std::endl;
    return UUAGENT_ZMQ_ERROR;
  }
}
