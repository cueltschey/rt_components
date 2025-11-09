CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
set(SPOOFER_SRC_DIR src)
set(SPOOFER_HDR_DIR hdr)

# Common source files (shared between executables)
set(COMMON_SOURCES
    "${SPOOFER_SRC_DIR}/config.cc"
    "${SPOOFER_SRC_DIR}/rf_handler.cc"
    "${SPOOFER_SRC_DIR}/ssb_processor.cc"
)

# Main SSB Spoofer executable
add_executable(ssb_spoofer 
    ${COMMON_SOURCES}
    "${SPOOFER_SRC_DIR}/main.cpp"
)

set_target_properties(ssb_spoofer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    LINKER_LANGUAGE CXX
)

target_include_directories(ssb_spoofer PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/${SPOOFER_HDR_DIR}"
)


set(SRSUE_SOURCES srsue_phy srsue_stack srsue_upper srsue_mac srsue_rrc srslog system)
set(SRSRAN_SOURCES srsran_common srsran_mac srsran_phy srsran_radio srsran_gtpu srsran_rlc srsran_pdcp rrc_asn1 srslog support system)

set(SRSUE_SOURCES ${SRSUE_SOURCES} srsue_nr_stack srsue_rrc_nr srsue_mac_nr)
set(SRSRAN_SOURCES ${SRSRAN_SOURCES} rrc_nr_asn1 ngap_nr_asn1)

target_link_libraries(ssb_spoofer   ${SRSUE_SOURCES}
																		${SRSRAN_SOURCES}
																		${CMAKE_THREAD_LIBS_INIT}
																		${Boost_LIBRARIES}
																		${ATOMIC_LIBS})

# SSB File Analyzer executable
add_executable(ssb_file_analyzer
    "${SPOOFER_SRC_DIR}/config.cc"
    "${SPOOFER_SRC_DIR}/ssb_processor.cc"
    "${SPOOFER_SRC_DIR}/ssb_file_analyzer.cpp"
)

set_target_properties(ssb_file_analyzer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    LINKER_LANGUAGE CXX
)

target_include_directories(ssb_file_analyzer PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/${SPOOFER_HDR_DIR}"
)

target_link_libraries(ssb_file_analyzer PRIVATE
    srsran_phch
    srsran_phy
    srsran_common
    support
    srslog
    pthread
)

install(TARGETS ssb_spoofer ssb_file_analyzer DESTINATION /usr/local/bin OPTIONAL)

