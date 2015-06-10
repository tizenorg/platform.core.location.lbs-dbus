CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(lbs-dbus-client C)

INCLUDE_DIRECTORIES(
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/cmake_tmp 
		${CMAKE_BINARY_DIR}
		)

### Build ###
SET(SRCS
	src/lbs_dbus_client.c
	${CMAKE_CURRENT_SOURCE_DIR}/generated-code.c
)

ADD_LIBRARY(lbs-dbus SHARED ${SRCS})
TARGET_LINK_LIBRARIES(lbs-dbus ${pkgs_LDFLAGS})
SET_TARGET_PROPERTIES(lbs-dbus PROPERTIES VERSION 0.1.0 SOVERSION 0 OUTPUT_NAME lbs-dbus)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/lbs_dbus_client.h DESTINATION include/lbs-dbus)
INSTALL(TARGETS lbs-dbus DESTINATION lib COMPONENT Runtime)

ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/generated-code.c
	COMMAND gdbus-codegen --interface-prefix org.tizen.lbs. --generate-c-code generated-code --c-generate-object-manager --c-namespace Lbs --generate-docbook generated-docs ${CMAKE_SOURCE_DIR}/introspection/lbs.xml ${CMAKE_SOURCE_DIR}/introspection/lbs_position.xml ${CMAKE_SOURCE_DIR}/introspection/lbs_batch.xml ${CMAKE_SOURCE_DIR}/introspection/lbs_satellite.xml ${CMAKE_SOURCE_DIR}/introspection/lbs_nmea.xml ${CMAKE_SOURCE_DIR}/introspection/lbs_gps_geofence.xml
	COMMENT "Generating GDBus .c/.h")
