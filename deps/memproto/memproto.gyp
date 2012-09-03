{
  'targets': [
    {
      'target_name': 'memproto',
      'type': 'static_library',
      'sources': [
        'memtext.h',
        'memtext.c',
        'memproto.h',
        'memproto.c',
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
    },
  ]
}


