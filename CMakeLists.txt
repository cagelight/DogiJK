cmake_minimum_required( VERSION 3.15 )
project( dogijk )

option( BUILD_CLIENT "Build client executable and required modules." ON )
option( BUILD_SERVER "Build server executable and required modules." ON )

enable_language( CXX )
set( CMAKE_CONFIGURATION_TYPES "Debug;Release;Native" )

set( CMAKE_CXX_FLAGS         "-std=c++20" )
set( CMAKE_CXX_FLAGS_DEBUG   "-Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Og -march=core2 -mtune=native -ggdb3 -D_DEBUG" )
set( CMAKE_CXX_FLAGS_RELEASE "-w -O2 -march=core2 -mtune=generic -D_DEBUG -DFINAL_BUILD" )
set( CMAKE_CXX_FLAGS_NATIVE  "-w -Os --fast-math -march=native -mtune=native -D_DEBUG -DFINAL_BUILD" )

if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE Native )
endif()
message( "Using build configuration: ${CMAKE_BUILD_TYPE}" )

if ( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
	set( CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install CACHE PATH "..." FORCE )
endif()

find_package( Threads REQUIRED )
find_package( PkgConfig REQUIRED )
if( BUILD_CLIENT OR BUILD_SERVER )
	pkg_check_modules( MINIZIP REQUIRED IMPORTED_TARGET minizip )
	pkg_check_modules( BULLET REQUIRED IMPORTED_TARGET bullet )
endif()
if( BUILD_CLIENT )
	pkg_check_modules( JPEG REQUIRED IMPORTED_TARGET libjpeg )
	pkg_check_modules( PNG  REQUIRED IMPORTED_TARGET libpng )
	pkg_check_modules( JXL  REQUIRED IMPORTED_TARGET libjxl )
	pkg_check_modules( SDL2 REQUIRED IMPORTED_TARGET sdl2 )
endif()

include_directories( "src" )
set( ARCH_STRING "x86_64" )
add_compile_definitions( ARCH_STRING="${ARCH_STRING}" )

# ================================
# BOTLIB
if( BUILD_CLIENT OR BUILD_SERVER )

	file( GLOB_RECURSE BOTLIB_FILES 
		"src/botlib/*.cc"
		"src/qcommon/q_shared.cc"
		"src/qcommon/q_math.cc"
		"src/qcommon/q_string.cc"
	)

	add_library( botlib STATIC ${BOTLIB_FILES} )
	target_compile_definitions( botlib PUBLIC BOTLIB )

endif()

# ================================

file( GLOB_RECURSE CLIENT_SERVER_COMMON_FILES 
	"src/qcommon/*.cc"
	"src/icarus/*.cc"
	"src/server/*.cc"
	"src/server/NPCNav/*.cc"
	"src/mp3code/*.cc"
	"src/sys/snapvector.cc"
	"src/sys/sys_main.cc"
	"src/sys/sys_event.cc"
	"src/sys/sys_log.cc"
	"src/sys/con_log.cc"
	"src/sys/sys_unix.cc"
	"src/sys/con_tty.cc"
	"src/nav/navmap.cc"
)

# ================================
# SERVER
if( BUILD_SERVER )
	
	file( GLOB_RECURSE SERVER_FILES 
		"src/rd-dedicated/*.cc"
		"src/null/*.cc"
	)

	add_executable( dogijkded ${CLIENT_SERVER_COMMON_FILES} ${SERVER_FILES} )
	target_compile_definitions( dogijkded PUBLIC _CONSOLE DEDICATED )
	target_link_libraries( dogijkded PUBLIC botlib Threads::Threads PkgConfig::MINIZIP ${CMAKE_DL_LIBS} )
	install( TARGETS dogijkded RUNTIME DESTINATION "." )

endif()

# ================================
# CLIENT
if( BUILD_CLIENT )

	file( GLOB_RECURSE CLIENT_FILES 
		"src/client/*.cc"
		"src/sdl/sdl_window.cc"
		"src/sdl/sdl_input.cc"
		"src/sdl/sdl_sound.cc"
	)

	add_executable( dogijk ${CLIENT_SERVER_COMMON_FILES} ${CLIENT_FILES} )
	target_link_libraries( dogijk PUBLIC botlib Threads::Threads PkgConfig::SDL2 PkgConfig::MINIZIP ${CMAKE_DL_LIBS} "-lavcodec -lavformat -lavutil -lswscale" )
	install( TARGETS dogijk RUNTIME DESTINATION "." )

endif()

# ================================
# GAME
if( BUILD_CLIENT OR BUILD_SERVER )

	set ( GAME_MODULE "jampgame${ARCH_STRING}")

	file( GLOB_RECURSE GAME_FILES 
		"src/qcommon/q_math.cc"
		"src/qcommon/q_string.cc"
		"src/qcommon/q_shared.cc"
		"src/qcommon/q_task.cc"
		"src/game/*.cc"
		"src/nav/*.cc"
	)

	add_library( ${GAME_MODULE} MODULE ${GAME_FILES} )
	target_compile_definitions( ${GAME_MODULE} PUBLIC _GAME )
	target_link_libraries( ${GAME_MODULE} PUBLIC PkgConfig::BULLET Threads::Threads "-lmeadow" )
	set_target_properties( ${GAME_MODULE} PROPERTIES PREFIX "" )
	install( TARGETS ${GAME_MODULE} LIBRARY DESTINATION "base/" )
	
endif()

# ================================
# CGAME
if( BUILD_CLIENT )

	set ( CGAME_MODULE "cgame${ARCH_STRING}")

	file( GLOB_RECURSE CGAME_FILES 
		"src/qcommon/q_math.cc"
		"src/qcommon/q_string.cc"
		"src/qcommon/q_shared.cc"
		"src/qcommon/q_task.cc"
		"src/cgame/*.cc"
		"src/game/bg_*.cc"
		"src/game/AnimalNPC.cc"
		"src/game/FighterNPC.cc"
		"src/game/SpeederNPC.cc"
		"src/game/WalkerNPC.cc"
		"src/ui/ui_shared.cc"
		"src/nav/*.cc"
	)

	add_library( ${CGAME_MODULE} MODULE ${CGAME_FILES} )
	target_compile_definitions( ${CGAME_MODULE} PUBLIC _CGAME )
	target_link_libraries( ${CGAME_MODULE} PUBLIC PkgConfig::BULLET Threads::Threads )
	set_target_properties( ${CGAME_MODULE} PROPERTIES PREFIX "" )
	install( TARGETS ${CGAME_MODULE} LIBRARY DESTINATION "base/" )

endif()

# ================================
# UI
if( BUILD_CLIENT )

	set ( UI_MODULE "ui${ARCH_STRING}")

	file( GLOB_RECURSE UI_FILES 
		"src/qcommon/q_math.cc"
		"src/qcommon/q_string.cc"
		"src/qcommon/q_shared.cc"
		"src/ui/*.cc"
		"src/game/bg_misc.cc"
		"src/game/bg_saberLoad.cc"
		"src/game/bg_saga.cc"
		"src/game/bg_vehicleLoad.cc"
		"src/game/bg_weapons.cc"
	)

	add_library( ${UI_MODULE} MODULE ${UI_FILES} )
	target_compile_definitions( ${UI_MODULE} PUBLIC UI_BUILD )
	target_link_libraries( ${UI_MODULE} PUBLIC Threads::Threads )
	set_target_properties( ${UI_MODULE} PROPERTIES PREFIX "" )
	install( TARGETS ${UI_MODULE} LIBRARY DESTINATION "base/" )

endif()

# ================================
# GHOUL2
if( BUILD_CLIENT OR BUILD_SERVER )

	set ( G2_MODULE "ghoul2_${ARCH_STRING}")

	file( GLOB_RECURSE G2_FILES 
		"src/qcommon/q_math.cc"
		"src/qcommon/q_string.cc"
		"src/qcommon/q_shared.cc"
		"src/qcommon/matcomp.cc"
		"src/ghoul2/*.cc"
	)

	add_library( ${G2_MODULE} MODULE ${G2_FILES} )
	target_link_libraries( ${G2_MODULE} PUBLIC Threads::Threads )
	set_target_properties( ${G2_MODULE} PROPERTIES PREFIX "" )
	install( TARGETS ${G2_MODULE} LIBRARY DESTINATION "." )

endif()

# ================================
# RD-VANILLA
if( BUILD_CLIENT )

	set ( RDVAN_MODULE "rd-vanilla_${ARCH_STRING}")

	file( GLOB_RECURSE RDVAN_FILES 
		"src/qcommon/q_math.cc"
		"src/qcommon/q_string.cc"
		"src/qcommon/q_shared.cc"
		"src/qcommon/q_simplex.cc"
		"src/qcommon/matcomp.cc"
		"src/rd-vanilla/*.cc"
		"src/rd-common/*.cc"
	)

	add_library( ${RDVAN_MODULE} MODULE ${RDVAN_FILES} )
	target_include_directories( ${RDVAN_MODULE} PUBLIC "src/rd-vanilla" )
	target_link_libraries( ${RDVAN_MODULE} PUBLIC Threads::Threads PkgConfig::JPEG PkgConfig::PNG PkgConfig::JXL )
	set_target_properties( ${RDVAN_MODULE} PROPERTIES PREFIX "" )
	install( TARGETS ${RDVAN_MODULE} LIBRARY DESTINATION "." )

endif()

# ================================
# RD-HOWLER
if( BUILD_CLIENT )

	set ( RDHOW_MODULE "rd-howler_${ARCH_STRING}")

	file( GLOB_RECURSE RDHOW_FILES 
		"src/qcommon/q_math.cc"
		"src/qcommon/q_string.cc"
		"src/qcommon/q_shared.cc"
		"src/qcommon/q_simplex.cc"
		"src/qcommon/matcomp.cc"
		"src/rd-howler/*.cc"
		"src/rd-common/*.cc"
	)

	add_library( ${RDHOW_MODULE} MODULE ${RDHOW_FILES} )
	target_include_directories( ${RDHOW_MODULE} PUBLIC "src/rd-howler" )
	target_link_libraries( ${RDHOW_MODULE} PUBLIC Threads::Threads PkgConfig::JPEG PkgConfig::PNG PkgConfig::JXL "-lbsp" )
	set_target_properties( ${RDHOW_MODULE} PROPERTIES PREFIX "" )
	install( TARGETS ${RDHOW_MODULE} LIBRARY DESTINATION "." )

endif()

# ================================
