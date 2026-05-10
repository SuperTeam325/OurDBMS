# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles\\DBMS2_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\DBMS2_autogen.dir\\ParseCache.txt"
  "DBMS2_autogen"
  )
endif()
