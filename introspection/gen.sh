gdbus-codegen --interface-prefix org.tizen.lbs. \
	--generate-c-code generated-code                        \
	--c-generate-object-manager                 \
	--c-namespace Lbs                        \
	--generate-docbook generated-docs                       \
	lbs.xml lbs_position.xml lbs_satellite.xml lbs_nmea.xml
