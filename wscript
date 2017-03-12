#!/bin/python

from waflib import *
import os, sys

top = os.getcwd()
out = 'build'

projname = 'dogibooru'
coreprog_name = projname

g_cflags = ["-Wall", "-Wextra", "-std=c++17"]
def btype_cflags(ctx):
	return {
		"DEBUG"   : g_cflags + ["-Og", "-ggdb3", "-march=core2", "-mtune=native"],
		"NATIVE"  : g_cflags + ["-Ofast", "-march=native", "-mtune=native"],
		"RELEASE" : g_cflags + ["-O3", "-march=core2", "-mtune=generic"],
	}.get(ctx.env.BUILD_TYPE, g_cflags)

def options(opt):
	opt.load("compiler_cxx qt5")
	opt.add_option('--build_type', dest='build_type', type="string", default='RELEASE', action='store', help="DEBUG, NATIVE, RELEASE")

def configure(ctx):
	ctx.load("compiler_cxx qt5")
	ctx.check(features='c cprogram', lib='blueshift_module', uselib_store='BSM')
	ctx.check(features='c cprogram', lib='stdc++fs', uselib_store='EFS') # should be temporary until C++17
	ctx.check(features='c cprogram', lib='pq', uselib_store='POSTGRES')
	btup = ctx.options.build_type.upper()
	if btup in ["DEBUG", "NATIVE", "RELEASE"]:
		Logs.pprint("PINK", "Setting up environment for known build type: " + btup)
		ctx.env.BUILD_TYPE = btup
		ctx.env.CXXFLAGS = btype_cflags(ctx)
		Logs.pprint("PINK", "CXXFLAGS: " + ' '.join(ctx.env.CXXFLAGS))
		if btup == "DEBUG":
			ctx.define("DOGIBOORU_DEBUG", 1)
	else:
		Logs.error("UNKNOWN BUILD TYPE: " + btup)
		
def build(bld):
	files =  bld.path.ant_glob('src/*.cc')
	coreprog = bld (
		features = "cxx cxxshlib qt5",
		target = coreprog_name,
		source = files,
		use = ['QT5CORE','QT5GUI'],
		uselib = ['BSM', 'EFS', 'POSTGRES'],
		includes = [os.path.join(top, 'src'), '/usr/local/include/blueshift'],
		install_path = os.path.join(top, 'install')
	)
	coreprog.env.cxxshlib_PATTERN = '%s'
