
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  execute_process (COMMAND bash -c "cd ${ASSET_ROOT}/data0_000_21/ && 7z a ${BIN_DIR}/basewf/data0_000_21.pk3 -r")
  execute_process (COMMAND bash -c "cd ${ASSET_ROOT}/data0_000_21pure/ && 7z a ${BIN_DIR}/basewf/data0_000_21pure.pk3 -r")
  execute_process (COMMAND bash -c "cd ${ASSET_ROOT}/data0_21/ && 7z a ${BIN_DIR}/basewf/data0_21.pk3 *")
  execute_process (COMMAND bash -c "cd ${ASSET_ROOT}/data0_21pure/ && 7z a ${BIN_DIR}/basewf/data0_21pure.pk3 -r")
  execute_process (COMMAND bash -c "cd ${ASSET_ROOT}/data1_21pure/ && 7z a ${BIN_DIR}/basewf/data1_21pure.pk3 -r")
else()
  execute_process (COMMAND bash -c "cd ${ASSET_ROOT}/data0_000_21/ && zip -r ${BIN_DIR}/basewf/data0_000_21.pk3 *")
  execute_process (COMMAND bash -c "cd ${ASSET_ROOT}/data0_000_21pure/ && zip -r ${BIN_DIR}/basewf/data0_000_21pure.pk3 *")
  execute_process (COMMAND bash -c "cd ${ASSET_ROOT}/data0_21/ && zip -r ${BIN_DIR}/basewf/data0_21.pk3 *")
  execute_process (COMMAND bash -c "cd ${ASSET_ROOT}/data0_21pure/ && zip -r ${BIN_DIR}/basewf/data0_21pure.pk3 *")
  execute_process (COMMAND bash -c "cd ${ASSET_ROOT}/data1_21pure/ && zip -r ${BIN_DIR}/basewf/data1_21pure.pk3 *")
endif()

file(COPY ${ASSET_ROOT}/profiles  DESTINATION ${BIN_DIR}/basewf/profiles)
file(COPY ${ASSET_ROOT}/configs DESTINATION ${BIN_DIR}/basewf/configs)
file(GLOB CONFIG_FILES 
  "${ASSET_ROOT}/*.cfg"
  "${ASSET_ROOT}/*.md"
)
file(COPY ${CONFIG_FILES} DESTINATION ${BIN_DIR}/basewf)
