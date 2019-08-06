add_library (veloc-client SHARED 
  veloc.cpp
  client.cpp
  ${VELOC_SOURCE_DIR}/src/common/config.cpp
)
find_package(Boost 1.53 COMPONENTS thread REQUIRED)

    target_link_libraries (veloc-client veloc-modules thallium Boost::thread ${ER_LIBRARIES} ${MPI_CXX_LIBRARIES} rt )

# Install libraries
install (TARGETS veloc-client
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install (FILES
  ${VELOC_SOURCE_DIR}/include/veloc.h
  DESTINATION include
)
