#!/bin/python

from waflib import *
import os, sys

top = os.getcwd()
out = 'build'
	
g_comflags = ['-pthread', '-Wall', '-Wextra', '-Wno-unused-variable', '-Wno-unused-parameter', '-Werror=invalid-offsetof']
g_cflags = ['-std=c99'] + g_comflags
g_cxxflags = ['-std=c++17'] + g_comflags

def btype_cflags(ctx):
	return {
		'DEBUG'   : g_cflags + ['-Og', '-ggdb3', '-march=core2', '-mtune=native'],
		'NATIVE'  : g_cflags + ['-Ofast', '-march=native', '-mtune=native'],
		'RELEASE' : g_cflags + ['-O3', '-march=core2', '-mtune=generic'],
	}.get(ctx.env.BUILD_TYPE, g_cflags)
def btype_cxxflags(ctx):
	return {
		'DEBUG'   : g_cxxflags + ['-Og', '-ggdb3', '-march=core2', '-mtune=native'],
		'NATIVE'  : g_cxxflags + ['-Ofast', '-march=native', '-mtune=native'],
		'RELEASE' : g_cxxflags + ['-O3', '-march=core2', '-mtune=generic'],
	}.get(ctx.env.BUILD_TYPE, g_cxxflags)

def options(opt):
	opt.load('gcc')
	opt.load('g++')
	
	opt.add_option('--build-type', dest='build_type', type='string', default='RELEASE', action='store', help='DEBUG, NATIVE, RELEASE')
	opt.add_option('--no-server', dest='bldsv', default=True, action='store_false', help='True/False')
	opt.add_option('--no-client', dest='bldcl', default=True, action='store_false', help='True/False')

def configure(ctx):
	
	ctx.env.BUILD_SERVER = ctx.options.bldsv
	ctx.env.BUILD_CLIENT = ctx.options.bldcl
	
	ctx.load('gcc')
	ctx.load('g++')
	
	ctx.check(features='c cprogram', lib='z', uselib_store='ZLIB')
	ctx.check(features='c cprogram', lib='dl', uselib_store='DL')
	ctx.check(features='c cprogram', lib='pthread', uselib_store='PTHREAD')
	
	if ctx.env.BUILD_CLIENT:
		ctx.check(features='c cprogram', lib='jpeg', uselib_store='JPEG')
		ctx.check(features='c cprogram', lib='png', uselib_store='PNG')
		ctx.check_cfg(path='sdl2-config', args='--cflags --libs', package='', uselib_store='SDL')
	
	ctx.check_cfg(path='pkg-config', args='--cflags --libs', package='bullet', uselib_store='BULLET')
	ctx.check(features='c cprogram', lib='mruby', uselib_store='MRUBY')
	
	btup = ctx.options.build_type.upper()
	if btup in ['DEBUG', 'NATIVE', 'RELEASE']:
		Logs.pprint('PINK', 'Setting up environment for known build type: ' + btup)
		ctx.env.BUILD_TYPE = btup
		ctx.env.CFLAGS = btype_cflags(ctx)
		ctx.env.CXXFLAGS = btype_cxxflags(ctx)
		Logs.pprint('PINK', 'CFLAGS: ' + ' '.join(ctx.env.CFLAGS))
		Logs.pprint('PINK', 'CXXFLAGS: ' + ' '.join(ctx.env.CXXFLAGS))
		if btup == 'DEBUG':
			ctx.define('_DEBUG', 1)
		else:
			ctx.define('NDEBUG', 1)
			ctx.define('FINAL_BUILD', 1)
		ctx.define('ARCH_STRING', 'x86_64')
	else:
		Logs.error('UNKNOWN BUILD TYPE: ' + btup)

def build(bld):
	
	build_server = bld.env.BUILD_SERVER
	build_client = bld.env.BUILD_CLIENT
	
	build_game = build_server or build_client
	build_cgame = build_client
	build_ui = build_client
	build_rdvan = build_client
	build_ghoul2 = build_client

	# MINIZIP
	if build_server or build_client:
		minizip_files = bld.path.ant_glob('src/minizip/*.c')
		minizip = bld (
			features = 'c cstlib',
			target = 'minizip',
			includes = 'src/minizip/include/minizip',
			source = minizip_files,
		)
	
	# BOTLIB
	if build_server or build_client:
		botlib_files = bld.path.ant_glob('src/botlib/*.cc')
		botlib_files += bld.path.ant_glob('src/qcommon/q_shared.cc')
		botlib_files += bld.path.ant_glob('src/qcommon/q_math.cc')
		botlib_files += bld.path.ant_glob('src/qcommon/q_string.cc')
		
		botlib = bld (
			features = 'cxx cxxstlib',
			target = 'botlib',
			includes = ['src'],
			source = botlib_files,
			defines = ['BOTLIB'],
		)
	
	clsv_files = []
	clsv_files += bld.path.ant_glob('src/qcommon/*.cc')
	clsv_files += bld.path.ant_glob('src/icarus/*.cc')
	clsv_files += bld.path.ant_glob('src/server/*.cc')
	clsv_files += bld.path.ant_glob('src/server/NPCNav/*.cc')
	clsv_files += bld.path.ant_glob('src/mp3code/*.cc')
	clsv_files += bld.path.ant_glob('src/sys/snapvector.cc')
	clsv_files += bld.path.ant_glob('src/sys/sys_main.cc')
	clsv_files += bld.path.ant_glob('src/sys/sys_event.cc')
	clsv_files += bld.path.ant_glob('src/sys/sys_log.cc')
	clsv_files += bld.path.ant_glob('src/sys/con_log.cc')
	clsv_files += bld.path.ant_glob('src/sys/sys_unix.cc')
	clsv_files += bld.path.ant_glob('src/sys/con_tty.cc')
	clsv_files += bld.path.ant_glob('src/ruby/*.cc')
	
	# SERVER
	if build_server:
	
		server_files = bld.path.ant_glob('src/rd-dedicated/*.cc')
		server_files += bld.path.ant_glob('src/null/*.cc')
		server_files += bld.path.ant_glob('src/ghoul2/*.cc')

		server = bld (
			features = 'cxx cxxprogram',
			target = 'dogijkded',
			includes = ['src', '/usr/include/tirpc'],
			source = clsv_files + server_files,
			defines = ['_CONSOLE', 'DEDICATED'],
			uselib = ['ZLIB', 'DL', 'PTHREAD', 'MRUBY'],
			use = ['minizip', 'botlib'],
			install_path = os.path.join(top, 'install')
		)
	
	# CLIENT
	if build_client:
	
		client_files = []
		client_files += bld.path.ant_glob('src/client/*.cc')
		client_files += bld.path.ant_glob('src/sdl/sdl_window.cc')
		client_files += bld.path.ant_glob('src/sdl/sdl_input.cc')
		client_files += bld.path.ant_glob('src/sdl/sdl_sound.cc')
		
		client = bld (
			features = 'cxx cxxprogram',
			target = 'dogijk',
			includes = ['src', '/usr/include/tirpc'],
			source = clsv_files + client_files,
			uselib = ['SDL', 'ZLIB', 'DL', 'PTHREAD', 'MRUBY'],
			use = ['minizip', 'botlib'],
			install_path = os.path.join(top, 'install')
		)
	
	gcgui_files = bld.path.ant_glob('src/qcommon/q_math.cc')
	gcgui_files += bld.path.ant_glob('src/qcommon/q_string.cc')
	gcgui_files += bld.path.ant_glob('src/qcommon/q_shared.cc')
	
	# GAME
	if build_game:
		game_files = bld.path.ant_glob('src/game/*.cc')
		
		game = bld (
			features = 'cxx cxxshlib',
			target = 'jampgame',
			includes = ['src'],
			source = gcgui_files + game_files,
			uselib = ['PTHREAD', 'BULLET'],
			defines = ['_GAME'],
			install_path = os.path.join(top, 'install', 'base')
		)
		
		game.env.cxxshlib_PATTERN = '%sx86_64.so'
	
	
	# CGAME
	if build_cgame:
		
		cgame_files = bld.path.ant_glob('src/cgame/*.cc')
		cgame_files += bld.path.ant_glob('src/game/bg_*.cc')
		cgame_files += bld.path.ant_glob('src/game/AnimalNPC.cc')
		cgame_files += bld.path.ant_glob('src/game/FighterNPC.cc')
		cgame_files += bld.path.ant_glob('src/game/SpeederNPC.cc')
		cgame_files += bld.path.ant_glob('src/game/WalkerNPC.cc')
		cgame_files += bld.path.ant_glob('src/ui/ui_shared.cc')
		
		cgame = bld (
			features = 'cxx cxxshlib',
			target = 'cgame',
			includes = ['src'],
			source = gcgui_files + cgame_files,
			uselib = ['PTHREAD', 'BULLET'],
			defines = ['_CGAME'],
			install_path = os.path.join(top, 'install', 'base')
		)
		
		cgame.env.cxxshlib_PATTERN = '%sx86_64.so'
		
	# UI
	if build_ui:
	
		ui_files = bld.path.ant_glob('src/ui/*.cc')
		ui_files += bld.path.ant_glob('src/game/bg_misc.cc')
		ui_files += bld.path.ant_glob('src/game/bg_saberLoad.cc')
		ui_files += bld.path.ant_glob('src/game/bg_saga.cc')
		ui_files += bld.path.ant_glob('src/game/bg_vehicleLoad.cc')
		ui_files += bld.path.ant_glob('src/game/bg_weapons.cc')
		
		ui = bld (
			features = 'cxx cxxshlib',
			target = 'ui',
			includes = ['src'],
			source = gcgui_files + ui_files,
			uselib = ['PTHREAD'],
			defines = ['UI_BUILD'],
			install_path = os.path.join(top, 'install', 'base')
		)
		
		ui.env.cxxshlib_PATTERN = '%sx86_64.so'
		
	# RD-VANILLA
	if build_rdvan:
	
		rdvan_files = bld.path.ant_glob('src/rd-vanilla/*.cc')
		rdvan_files += bld.path.ant_glob('src/rd-common/*.cc')
		rdvan_files += bld.path.ant_glob('src/qcommon/matcomp.cc')
		rdvan_files += bld.path.ant_glob('src/qcommon/q_shared.cc')
		rdvan_files += bld.path.ant_glob('src/qcommon/q_math.cc')
		rdvan_files += bld.path.ant_glob('src/qcommon/q_string.cc')
			
		rdvan = bld (
			features = 'cxx cxxshlib',
			target = 'rd-vanilla',
			includes = ['src', 'src/rd-vanilla'],
			source = rdvan_files,
			uselib = ['JPEG', 'PNG', 'PTHREAD'],
			install_path = os.path.join(top, 'install')
		)
		
		rdvan.env.cxxshlib_PATTERN = '%s_x86_64.so'
		
	# RD-HOWLER
	if build_rdvan:
	
		howler_files = bld.path.ant_glob('src/rd-howler/*.cc')
		howler_files += bld.path.ant_glob('src/rd-common/*.cc')
		howler_files += bld.path.ant_glob('src/qcommon/matcomp.cc')
		howler_files += bld.path.ant_glob('src/qcommon/q_shared.cc')
		howler_files += bld.path.ant_glob('src/qcommon/q_math.cc')
		howler_files += bld.path.ant_glob('src/qcommon/q_string.cc')
			
		rdvan = bld (
			features = 'cxx cxxshlib',
			target = 'rd-howler',
			includes = ['src', 'src/rd-howler'],
			source = howler_files,
			uselib = ['JPEG', 'PNG', 'PTHREAD'],
			install_path = os.path.join(top, 'install')
		)
		
		rdvan.env.cxxshlib_PATTERN = '%s_x86_64.so'
		
	# GHOUL2
	if build_ghoul2:
	
		g2_files = bld.path.ant_glob('src/ghoul2/*.cc')
		g2_files += bld.path.ant_glob('src/qcommon/matcomp.cc')
		g2_files += bld.path.ant_glob('src/qcommon/q_shared.cc')
		g2_files += bld.path.ant_glob('src/qcommon/q_math.cc')
		g2_files += bld.path.ant_glob('src/qcommon/q_string.cc')
			
		g2 = bld (
			features = 'cxx cxxshlib',
			target = 'ghoul2',
			includes = ['src', 'src/ghoul2'],
			source = g2_files,
			uselib = ['PTHREAD'],
			install_path = os.path.join(top, 'install')
		)
		
		g2.env.cxxshlib_PATTERN = '%s.so'
		
def clean(ctx):
	pass
