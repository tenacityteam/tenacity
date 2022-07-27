add_library(${TARGET} STATIC)
def_vars()

# Configure libmad's FPM
# TODO: implement more architectures other than x86_64 and i386
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64|x86_64")
   set(FPM_64BIT ON)
   message(STATUS "Using FPM 'FPM_64BIT' for selected target architecture")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i386|i686|x86")
   set(FPM_INTEL ON)
   message(STATUS "Using FPM 'FPM_INTEL' for selected target architecture")
else()
   set(FPM_DEFAULT ON)
   message(STATUS "Using default FPM for selected target architecture")
endif()

configure_file(${TARGET_ROOT}/mad.h.in ${TARGET_ROOT}/mad.h)

list(APPEND SOURCES
   PRIVATE
      ${TARGET_ROOT}/bit.c
      ${TARGET_ROOT}/decoder.c
      ${TARGET_ROOT}/fixed.c
      ${TARGET_ROOT}/frame.c
      ${TARGET_ROOT}/huffman.c
      ${TARGET_ROOT}/layer12.c
      ${TARGET_ROOT}/layer3.c
      ${TARGET_ROOT}/stream.c
      ${TARGET_ROOT}/synth.c
      ${TARGET_ROOT}/timer.c
      ${TARGET_ROOT}/version.c
)

list(APPEND INCLUDES
   PUBLIC
      ${TARGET_ROOT}/huffman.h
      ${TARGET_ROOT}/layer3.h
      ${TARGET_ROOT}/global.h
      ${TARGET_ROOT}/layer12.h
   PRIVATE
      ${TARGET_ROOT}/mad.h
)

organize_source("${TARGET_ROOT}" "" "${SOURCES}")
target_sources(${TARGET} PRIVATE ${SOURCES})
target_include_directories(${TARGET} PRIVATE ${TARGET_ROOT})
