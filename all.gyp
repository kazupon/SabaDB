{
  'targets': [
    {
      'target_name': 'all',
      'type': 'none',
      'dependencies': [
        'sabadb.gyp:test',
        'sabadb.gyp:benchmark',
        'sabadb.gyp:sabadb'
      ]
    }
  ]
}
