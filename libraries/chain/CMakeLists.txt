configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/eosio/chain/core_symbol.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/eosio/chain/core_symbol.hpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/genesis_state_root_key.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/genesis_state_root_key.cpp)

file(GLOB HEADERS "include/eosio/chain/*.hpp"
                  "include/eosio/chain/webassembly/*.hpp"
                  "${CMAKE_CURRENT_BINARY_DIR}/include/eosio/chain/core_symbol.hpp" )

if(APPLE AND UNIX)
   set(PLATFORM_TIMER_IMPL platform_timer_macos.cpp)
else()
   try_run(POSIX_TIMER_TEST_RUN_RESULT POSIX_TIMER_TEST_COMPILE_RESULT ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/platform_timer_posix_test.c)
   if(POSIX_TIMER_TEST_RUN_RESULT EQUAL 0)
      set(PLATFORM_TIMER_IMPL platform_timer_posix.cpp)
      set(CHAIN_RT_LINKAGE rt)
   else()
      set(PLATFORM_TIMER_IMPL platform_timer_asio_fallback.cpp)
   endif()
endif()

if("eos-vm-oc" IN_LIST EOSIO_WASM_RUNTIMES)
   enable_language(ASM)
   set(CHAIN_EOSVMOC_SOURCES webassembly/runtimes/eos-vm-oc/code_cache.cpp
                             webassembly/runtimes/eos-vm-oc/executor.cpp
                             webassembly/runtimes/eos-vm-oc/memory.cpp
                             webassembly/runtimes/eos-vm-oc/intrinsic.cpp
                             webassembly/runtimes/eos-vm-oc/LLVMJIT.cpp
                             webassembly/runtimes/eos-vm-oc/LLVMEmitIR.cpp
                             webassembly/runtimes/eos-vm-oc/compile_monitor.cpp
                             webassembly/runtimes/eos-vm-oc/compile_trampoline.cpp
                             webassembly/runtimes/eos-vm-oc/ipc_helpers.cpp
                             webassembly/runtimes/eos-vm-oc/gs_seg_helpers.c
                             webassembly/runtimes/eos-vm-oc/stack.cpp
                             webassembly/runtimes/eos-vm-oc/switch_stack_linux.s
                             webassembly/runtimes/eos-vm-oc.cpp
                             webassembly/runtimes/eos-vm-oc/default_real_main.cpp)

   if(LLVM_VERSION VERSION_LESS 7.1 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      enable_language(ASM-LLVMWAR)
      list(APPEND CHAIN_EOSVMOC_SOURCES webassembly/runtimes/eos-vm-oc/llvmWARshim.llvmwar)
   else()
      list(APPEND CHAIN_EOSVMOC_SOURCES webassembly/runtimes/eos-vm-oc/llvmWARshim.cpp)
   endif()

   llvm_map_components_to_libnames(LLVM_LIBS support core passes mcjit native orcjit)
   include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
   add_definitions(${LLVM_DEFINITIONS})

   option(EOSVMOC_ENABLE_DEVELOPER_OPTIONS "enable developer options for EOS VM OC" OFF)
endif()

if("eos-vm" IN_LIST EOSIO_WASM_RUNTIMES OR "eos-vm-jit" IN_LIST EOSIO_WASM_RUNTIMES)
   set(CHAIN_EOSVM_SOURCES "webassembly/runtimes/eos-vm.cpp")
   set(CHAIN_EOSVM_LIBRARIES eos-vm)
endif()

set(CHAIN_WEBASSEMBLY_SOURCES
   webassembly/action.cpp
   webassembly/authorization.cpp
   webassembly/cf_system.cpp
   webassembly/cf_transaction.cpp
   webassembly/compiler_builtins.cpp
   webassembly/context_free.cpp
   webassembly/console.cpp
   webassembly/crypto.cpp
   webassembly/database.cpp
   webassembly/memory.cpp
   webassembly/permission.cpp
   webassembly/privileged.cpp
   webassembly/producer.cpp
   webassembly/softfloat.cpp
   webassembly/system.cpp
   webassembly/transaction.cpp
)

add_library(eosio_rapidjson INTERFACE)
target_include_directories(eosio_rapidjson INTERFACE ../rapidjson/include)

## SORT .cpp by most likely to change / break compile
add_library( eosio_chain
             merkle.cpp
             name.cpp
             transaction.cpp
             block.cpp
             block_header.cpp
             block_header_state.cpp
             block_state.cpp
             fork_database.cpp
             controller.cpp
             authorization_manager.cpp
             resource_limits.cpp
             block_log.cpp
             transaction_context.cpp
             eosio_contract.cpp
             eosio_contract_abi.cpp
             eosio_contract_abi_bin.cpp
             chain_config.cpp
             chain_id_type.cpp
             genesis_state.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/genesis_state_root_key.cpp

              wast_to_wasm.cpp
              wasm_interface.cpp
              wasm_eosio_validation.cpp
              wasm_eosio_injection.cpp
              wasm_config.cpp
              apply_context.cpp
              abi_serializer.cpp
              asset.cpp
              snapshot.cpp
              deep_mind.cpp

             ${CHAIN_EOSVMOC_SOURCES}
             ${CHAIN_EOSVM_SOURCES}
             ${CHAIN_WEBASSEMBLY_SOURCES}

             authority.cpp
             trace.cpp
             transaction_metadata.cpp
             protocol_state_object.cpp
             protocol_feature_activation.cpp
             protocol_feature_manager.cpp
             producer_schedule.cpp
             genesis_intrinsics.cpp
             whitelisted_intrinsics.cpp
             thread_utils.cpp
             platform_timer_accuracy.cpp
             ${PLATFORM_TIMER_IMPL}
             ${HEADERS}
             )

target_link_libraries( eosio_chain PUBLIC bn256 fc chainbase eosio_rapidjson Logging IR WAST WASM
                       softfloat builtins ${CHAIN_EOSVM_LIBRARIES} ${LLVM_LIBS} ${CHAIN_RT_LINKAGE}
                     )
target_include_directories( eosio_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../wasm-jit/Include"
                            )

add_library(eosio_chain_wrap INTERFACE )
target_link_libraries(eosio_chain_wrap INTERFACE eosio_chain)

if("eos-vm-oc" IN_LIST EOSIO_WASM_RUNTIMES)
   target_link_libraries(eosio_chain_wrap INTERFACE "-Wl,-wrap=main")
endif()

foreach(RUNTIME ${EOSIO_WASM_RUNTIMES})
   string(TOUPPER "${RUNTIME}" RUNTIMEUC)
   string(REPLACE "-" "_" RUNTIMEUC ${RUNTIMEUC})
   target_compile_definitions(eosio_chain PUBLIC "EOSIO_${RUNTIMEUC}_RUNTIME_ENABLED")
endforeach()

if(EOSVMOC_ENABLE_DEVELOPER_OPTIONS)
   message(WARNING "EOS VM OC Developer Options are enabled; these are NOT supported")
   target_compile_definitions(eosio_chain PUBLIC EOSIO_EOS_VM_OC_DEVELOPER)
endif()

install( TARGETS eosio_chain
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} COMPONENT dev EXCLUDE_FROM_ALL
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT dev EXCLUDE_FROM_ALL
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT dev EXCLUDE_FROM_ALL
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/eosio/chain/
      DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/eosio/chain
      COMPONENT dev EXCLUDE_FROM_ALL
      FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" EXCLUDE
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/eosio/chain/core_symbol.hpp DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/eosio/chain COMPONENT dev EXCLUDE_FROM_ALL)
