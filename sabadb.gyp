{
  'variables': {
    # TODO:
    #'sabadb_shared_kc%': 'true',
    'kc_shared_include_dir': '<(module_root_dir)/deps/kyotocabinet',
    'kc_shared_library': '<(module_root_dir)/deps/kyotocabinet/libkyotocabinet.a',
    'sabadb_shared_cunit%': 'false',
  },

  'targets': [{
    'target_name': 'kyotocabinet',
    'type': 'none',
    'actions': [{
      'action_name': 'test',
      'inputs': ['<!@(sh kc-config.sh)'],
      'outputs': [''],
      'conditions': [[
        'OS=="win"', {
          'action': [
            'echo', 'notsupport'
          ]
        }, {
          'action': [
            # run kyotocabinet `make`
            'sh', 'kc-build.sh'
          ]
        }
      ]]
    }]
  }, {
    'target_name': 'libsabadb',
    'type': 'static_library',
    'dependencies': [
      'deps/libuv/uv.gyp:uv',
      'kyotocabinet',
    ],
    'export_dependent_settings': [
      'deps/libuv/uv.gyp:uv',
    ],
    'sources': [
      'src/saba_utils.c',
      'src/saba_message.c',
      'src/saba_message_queue.c',
      'src/saba_logger.c',
      'src/saba_worker.c',
      'src/saba_server.c',
    ],
    'defines': [
      'LIBUV_VERSION="<!(git --git-dir deps/libuv/.git describe --all --tags --always --long)"',
      'KC_VERSION="<!(git --git-dir deps/kyotocabinet/.git describe --all --tags --always --long)"',
    ],
    'include_dirs': [
      'src',
      'deps/libuv/include',
      '<(kc_shared_include_dir)',
    ],
    'libraries': [
      '<(kc_shared_library)',
    ], 
    'conditions': [
      [
        'OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'cflags': [ '--std=c99' ],
          'ldflags': [ '-lz', '-lstdc++', '-lrt', '-lpthread', '-lm', '-lc' ],
          'defines': [ '_GNU_SOURCE' ]
        }
      ], [
        'OS=="mac"', {
          'xcode_settings': {
            'GCC_C_LANGUAGE_STANDARD': 'c99',
          },
          'link_settings': {
            'libraries': [
              'libz.dylib',
              'libstdc++.dylib',
              'libpthread.dylib',
              'libm.dylib',
              'libc.dylib',
            ],
          },
          'defines': [
            '_DARWIN_USE_64_BIT_INODE=1',
          ],
        }
      ]
    ],
  }, {
    'target_name': 'test',
    'type': 'executable',
    'dependencies': [
      'libsabadb',
    ],
    'sources': [
      'test/test_saba_utils.c',
      'test/test_saba_message_queue.c',
      'test/test_saba_logger.c',
      'test/test_saba_worker.c',
      'test/test_saba_server.c',
      'test/runner.c',
    ],
    'include_dirs': [
      'src',
      '<(kc_shared_include_dir)',
    ],
    'conditions': [
      [
        'sabadb_shared_cunit=="false"', {
          'include_dirs': [
            '/usr/local/include',
          ],
          'libraries': [
            '/usr/local/lib',
          ], 
        }
      ], [
        'OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'cflags': [ '--std=c99' ],
          'defines': [ '_GNU_SOURCE' ]
        }
      ], [
        'OS=="mac"', {
          'xcode_settings': {
            'GCC_C_LANGUAGE_STANDARD': 'c99',
          },
          'defines': [
            '_DARWIN_USE_64_BIT_INODE=1',
          ],
        },
      ],
    ],
  }, {
    'target_name': 'benchmark',
    'type': 'executable',
    'dependencies': [
      'libsabadb',
    ],
    'sources': [
      'benchmark/roundtrips.c',
    ],
    'include_dirs': [
      'src',
      '<(kc_shared_include_dir)',
    ],
    'conditions': [
      [
        'OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'cflags': [ '--std=c99' ],
          'defines': [ '_GNU_SOURCE' ]
        }
      ], [
        'OS=="mac"', {
          'xcode_settings': {
            'GCC_C_LANGUAGE_STANDARD': 'c99',
          },
          'defines': [
            '_DARWIN_USE_64_BIT_INODE=1',
          ],
        },
      ],
    ],
  }, {
    'target_name': 'sabadb',
    'type': 'executable',
    'dependencies': [
      'libsabadb',
    ],
    'sources': [
      'src/main.c',
    ],
    'include_dirs': [
      'src',
      '<(kc_shared_include_dir)',
    ],
    'conditions': [
      [
        'OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'cflags': [ '--std=c99' ],
          'defines': [ '_GNU_SOURCE' ]
        }
      ], [
        'OS=="mac"', {
          'xcode_settings': {
            'GCC_C_LANGUAGE_STANDARD': 'c99',
          },
          'defines': [
            '_DARWIN_USE_64_BIT_INODE=1',
          ],
        },
      ],
    ],
  }]
}
