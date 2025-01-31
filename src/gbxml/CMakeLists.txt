set(target_name openstudio_gbxml)

set(${target_name}_src
  gbXMLAPI.hpp
  mainpage.hpp
  ForwardTranslator.hpp
  ForwardTranslator.cpp
  ReverseTranslator.hpp
  ReverseTranslator.cpp
  MapEnvelope.cpp
  MapSchedules.cpp
)

set(${target_name}_test_src
  Test/gbXMLFixture.hpp
  Test/gbXMLFixture.cpp
  Test/ForwardTranslator_GTest.cpp
  Test/ReverseTranslator_GTest.cpp
)

set(${target_name}_swig_src
  gbXML.i
)

file(GLOB_RECURSE XSD_SCHEMA_FILES  FOLLOW_SYMLINKS "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.xsd")
foreach(_FILE ${XSD_SCHEMA_FILES})
  file(RELATIVE_PATH LOCATION "${CMAKE_CURRENT_SOURCE_DIR}" ${_FILE})
  list(APPEND E_FILES ${_FILE})
  list(APPEND E_PATHS ${LOCATION})
endforeach()

include("${PROJECT_SOURCE_DIR}/embedded/EmbedFiles.cmake")
embed_files("${E_FILES}" "${E_PATHS}" EMBEDDED_OUTPUT openstudiogbxml)

# set up groups of source files for Visual Studio
source_group(embedded FILES ${EMBEDDED_OUTPUT})

set(${target_name}_depends
  ${openstudio_energyplus_depends}
)
set(${target_name}_depends ${${target_name}_depends} PARENT_SCOPE)

add_library(${target_name} OBJECT ${${target_name}_src} ${EMBEDDED_OUTPUT})
target_link_libraries(${target_name} PUBLIC ${${target_name}_depends})
target_compile_definitions(${target_name} PRIVATE openstudio_gbxml_EXPORTS SHARED_OS_LIBS)
target_include_directories(${target_name} PUBLIC ${PROJECT_BINARY_DIR})
add_dependencies(${target_name} GenerateIddFactoryRun)


set(${target_name}_test_depends
  openstudiolib
)

#CREATE_SRC_GROUPS("${${target_name}_src}")
#CREATE_SRC_GROUPS("${${target_name}_test_src}")
#CREATE_SRC_GROUPS("${${target_name}_swig_src}")

if(BUILD_TESTING)
  CREATE_TEST_TARGETS(${target_name} "${${target_name}_test_src}" "${${target_name}_test_depends}")
  add_dependencies(${target_name}_tests openstudio_gbxml_resources)
endif()

MAKE_SWIG_TARGET(OpenStudioGBXML gbXML "${CMAKE_CURRENT_SOURCE_DIR}/gbXML.i" "${${target_name}_swig_src}" ${target_name} OpenStudioEnergyPlus)
