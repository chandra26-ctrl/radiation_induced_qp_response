cmake_minimum_required(VERSION 3.12)

project(g4cmpChargePhononQPCascade LANGUAGES CXX)

# G4CMP supplies its package configuration through G4CMPINSTALL.
if(NOT DEFINED ENV{G4CMPINSTALL} OR "$ENV{G4CMPINSTALL}" STREQUAL "")
  message(FATAL_ERROR
    "G4CMPINSTALL is not set. Source the G4CMP environment setup script "
    "before configuring this project.")
endif()

list(PREPEND CMAKE_PREFIX_PATH "$ENV{G4CMPINSTALL}/../..")
find_package(G4CMP REQUIRED)

# Use the Geant4 configuration selected by the G4CMP installation.
include("$ENV{G4CMPINSTALL}/FindGeant4.cmake")
include(${G4CMP_USE_FILE})
include(${Geant4_USE_FILE})

# Match the C++ standard used to build Geant4 when one was not requested.
if(NOT DEFINED CMAKE_CXX_STANDARD)
  if(DEFINED Geant4_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD ${Geant4_CXX_STANDARD})
  elseif(DEFINED Geant4_CXXSTD)
    string(REPLACE "c++" "" CMAKE_CXX_STANDARD "${Geant4_CXXSTD}")
  else()
    set(CMAKE_CXX_STANDARD 17)
  endif()
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CASCADE_SOURCES
  src/ChargeActionInitialization.cc
  src/ChargeConfigManager.cc
  src/ChargeConfigMessenger.cc
  src/ChargeDetectorConstruction.cc
  src/ChargeElectrodeSensitivity.cc
  src/ChargePrimaryGeneratorAction.cc
  src/ChargeRunAction.cc
  src/ChargeSteppingAction.cc
)

add_executable(g4cmpCharge
  g4cmpCharge.cc
  ${CASCADE_SOURCES}
)

target_include_directories(g4cmpCharge PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${G4CMP_INCLUDE_DIRS}
)

target_link_libraries(g4cmpCharge PRIVATE
  ${G4CMP_LIBRARIES}
  ${Geant4_LIBRARIES}
)

# Make the run macros available directly from the build directory.
set(CASCADE_MACROS
  phonon_caustics.mac
  vis_eh.mac
)

foreach(macro IN LISTS CASCADE_MACROS)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${macro}
    ${CMAKE_CURRENT_BINARY_DIR}/${macro}
    COPYONLY
  )
endforeach()

install(TARGETS g4cmpCharge DESTINATION bin)
install(FILES ${CASCADE_MACROS}
        DESTINATION share/${PROJECT_NAME}/macros)
