#!/usr/bin/env python
# encoding: utf-8 syntax: python

import subprocess
import sys
import waflib
from glob import glob

APPNAME="cpp-base"
VERSION="0.1"

def options(ctx):
  ctx.load('compiler_cxx')
  ctx.add_option('--debug', action='store_true', default=False,
                  help='Build in debug mode')
  ctx.add_option('--compiler', action='store', default='',
                  help='Select compiler to use (clang++, g++)')

def configure(ctx):
  compiler = ctx.options.compiler

  if not compiler:
    if sys.platform == 'darwin':
      compiler = 'clang++'
    elif sys.platform == 'linux2':
      compiler = 'g++'
    else:
      print 'Platform %s not supported, things may break' % sys.platform

  if compiler:
    ctx.env.CXX = compiler
  ctx.load('compiler_cxx')

  flags = ['-std=c++0x', '-fno-rtti', '-fno-exceptions']

  if compiler == 'clang++':
    if sys.platform == 'darwin':
      flags += ['-stdlib=libc++']
      ctx.env.LINKFLAGS = ['-stdlib=libc++']
  elif compiler == 'g++':
    ctx.env.LIB = ['pthread']

  if ctx.options.debug:
    flags += ['-g', '-O0']
  else:
    flags += ['-O2', '-DNDEBUG']

  ctx.env.CXXFLAGS = flags
  ctx.env.CXXFLAGS_BASE = ['-Wall', '-Wextra', '-Wno-unused-parameter',
                           '-fstrict-aliasing', '-Woverloaded-virtual',
                           '-Werror']
  ctx.env.CXXFLAGS_3RD_PARTY = ['-w']
  ctx.env.CXXFLAGS_TESTS = ['-DGTEST_USE_OWN_TR1_TUPLE=1', '-DGTEST_HAS_RTTI=0']

def build(ctx):
  ctx.recurse('third_party')
  ctx.recurse('src/base')

def tags(ctx):
  subprocess.call(['ctags', '--extra=+f', '-R', 'src', 'third_party'])
