#!/usr/bin/env python
# encoding: utf-8 syntax: python

import subprocess
import sys
from glob import glob

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

  ctx.env.CXXFLAGS_LOCAL = flags + ['-Wall', '-Wextra', '-Wno-unused-parameter',
      '-fstrict-aliasing', '-Woverloaded-virtual', '-Werror']
  ctx.env.CXXFLAGS_3RD_PARTY = flags + ['-w']

def build(ctx):
  ctx(name = 'common',
      export_includes = 'src/')

  test_includes = 'third_party/googletest third_party/googletest/include ' \
                  'third_party/googlemock third_party/googlemock/include '
  test_flags = ['-DGTEST_USE_OWN_TR1_TUPLE=1', '-DGTEST_HAS_RTTI=0']

  ctx.stlib(target = 'googletest',
            includes = test_includes,
            export_includes = 'third_party/googletest/include',
            cxxflags = ctx.env.CXXFLAGS_3RD_PARTY + test_flags,
            source = 'third_party/googletest/src/gtest-all.cc')

  ctx.stlib(target = 'googlemock',
            includes = test_includes,
            export_includes = 'third_party/googlemock/include',
            cxxflags = ctx.env.CXXFLAGS_3RD_PARTY + test_flags,
            source = 'third_party/googlemock/src/gmock-all.cc')

  ctx.stlib(target = 'base',
            use = 'common',
            cxxflags = ctx.env.CXXFLAGS_LOCAL,
            source = 'src/base/file.cc '
                     'src/base/logging.cc '
                     'src/base/socket.cc '
                     'src/base/stack_trace.cc '
                     'src/base/time.cc '
                     'src/base/thread_checker.cc ')

  ctx.stlib(target = 'tests_common',
            use = 'common base googletest googlemock',
            cxxflags = ctx.env.CXXFLAGS_LOCAL + test_flags,
            source = 'src/base/unittest.cc ')

  ctx.program(target = 'base_tests',
              use = 'tests_common',
              cxxflags = ctx.env.CXXFLAGS_LOCAL + test_flags,
              source = 'src/base/bind_unittest.cc '
                       'src/base/logging_unittest.cc '
                       'src/base/stack_trace_unittest.cc '
                       'src/base/thread_checker_unittest.cc '
                       'src/base/weak_unittest.cc ')

def tags(ctx):
  subprocess.call(['ctags', '--extra=+f', '-R', 'src', 'third_party'])
