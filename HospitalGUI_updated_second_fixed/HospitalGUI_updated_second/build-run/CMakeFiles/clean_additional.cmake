# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  [[CMakeFiles\HospitalGUI_autogen.dir\AutogenUsed.txt]]
  [[CMakeFiles\HospitalGUI_autogen.dir\ParseCache.txt]]
  "HospitalGUI_autogen"
  )
endif()
