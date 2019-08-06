add_executable (veloc-backend
  main.cpp 
  ${VELOC_SOURCE_DIR}/src/common/config.cpp
)

# Link the executable to the necessary libraries.
find_package(Boost 1.53 COMPONENTS thread REQUIRED)

target_link_libraries (veloc-backend veloc-modules thallium  ${ER_LIBRARIES} Boost::thread ${MPI_CXX_LIBRARIES} rt )

# Link the executable to the necessary libraries.

# Install executables
install (TARGETS veloc-backend
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
