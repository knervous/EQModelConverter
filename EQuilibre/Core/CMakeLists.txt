find_file(EQEMU_OPCODE_FILE patch_Titanium.conf
          PATHS "${EQLI_SOURCE_PATH}"
          DOCS "Path to an EQEmu opcode file.")
if(NOT EXISTS ${EQEMU_OPCODE_FILE})
    message(FATAL_ERROR "EQEMU_OPCODE_FILE must be set.")
endif(NOT EXISTS ${EQEMU_OPCODE_FILE})

set(SCRIPT_DIR "${EQLI_SOURCE_PATH}/scripts")

# Generate a header file with all of the application opcodes.
set(MESSAGE_TYPES_HEADER "${CMAKE_CURRENT_BINARY_DIR}/MessageTypes.h")
generate_table(message_types "${EQEMU_OPCODE_FILE}" "${MESSAGE_TYPES_HEADER}")

# Generate a header file with all of the races and their animation mappings.
set(RACES_HEADER "${CMAKE_CURRENT_BINARY_DIR}/Races.def")
generate_table(csv_table "${EQLI_SOURCE_PATH}/races.csv" "${RACES_HEADER}" RACE knssss)

# Generate a header file with all of the zone names and other information.
set(ZONES_HEADER "${CMAKE_CURRENT_BINARY_DIR}/Zones.def")
generate_table(csv_table "${EQLI_SOURCE_PATH}/zones.csv" "${ZONES_HEADER}" ZONE kssiffffffffiiif)

add_custom_target(EQuilibreTables ALL DEPENDS
    ${MESSAGE_TYPES_HEADER}
    ${RACES_HEADER}
    ${ZONES_HEADER}
)
