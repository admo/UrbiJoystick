set(URBI_ROOT_DIR "/usr" CACHE PATH "Urbi root directory.")

find_package (Urbi REQUIRED)
find_package (SDL REQUIRED)
find_package (Boost REQUIRED thread date_time)

link_directories (${BOOST_LIBRARYDIR})

include_directories (${SDL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${URBI_INCLUDE_DIRS})

if (WIN32)
  add_definitions( -DBOOST_ALL_DYN_LINK )
endif (WIN32)

add_library (ujoystick SHARED ujoystick.cpp)

target_link_libraries (ujoystick ${SDL_LIBRARY} ${URBI_LIBRARIES} ${Boost_LIBRARIES})

set_target_properties (ujoystick PROPERTIES
  VERSION 0.0.1
  SOVERSION 0.0.1)

install (TARGETS ujoystick DESTINATION lib/gostai/uobjects COMPONENT libraries)
