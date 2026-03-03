#######################################

find_package(glfw3 CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)
find_package(Intl REQUIRED)

set(THIRD_PARTY_LIBS
	raylib
	glfw
	Intl::Intl
)

#######################################
