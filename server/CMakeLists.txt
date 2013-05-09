CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(lbs-dbus-server C)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/cmake_tmp ${CMAKE_BINARY_DIR})

### Build ###
SET(SRCS_S
	src/lbs_dbus_server.c
	${CMAKE_CURRENT_SOURCE_DIR}/generated-code.c
)
MESSAGE(Server_CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR})
MESSAGE(CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR})
MESSAGE(CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR})

ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/generated-code.c
	COMMAND gdbus-codegen --interface-prefix org.tizen.lbs. --generate-c-code generated-code --c-generate-object-manager --c-namespace Lbs --generate-docbook generated-docs ${CMAKE_SOURCE_DIR}/introspection/lbs.xml ${CMAKE_SOURCE_DIR}/introspection/lbs_position.xml ${CMAKE_SOURCE_DIR}/introspection/lbs_satellite.xml ${CMAKE_SOURCE_DIR}/introspection/lbs_nmea.xml
	COMMENT "Generating GDBus .c/.h")

ADD_LIBRARY(lbs-dbus-server SHARED ${SRCS_S})
TARGET_LINK_LIBRARIES(lbs-dbus-server ${pkgs_LDFLAGS})
SET_TARGET_PROPERTIES(lbs-dbus-server PROPERTIES VERSION 0.1.0 SOVERSION 0 OUTPUT_NAME lbs-dbus-server)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/lbs-dbus-server.conf DESTINATION ${PREFIX}/etc/dbus-1/system.d)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/lbs_dbus_server.h DESTINATION include/lbs-dbus)
INSTALL(TARGETS lbs-dbus-server DESTINATION lib COMPONENT Runtime)
#ADD_SUBDIRECTORY(test_src)
