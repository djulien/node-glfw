{
  'variables': {
    'platform': '<(OS)',
    'variant': 'default',
    'bcm_host': '<!(node -e "require(\\"fs\\").existsSync(\\"/opt/vc/include/bcm_host.h\\")")',
  },
  'conditions': [
    # Replace gyp platform with node platform, blech
    ['platform == "mac"', {'variables': {
      'platform': 'darwin',
      'ANTTWEAKBAR_ROOT': '/usr/local/Cellar/anttweakbar/1.16',
    }}],
    ['platform == "win"', {'variables': {'platform': 'win32'}}],
    # Detect Raspberry PI
    ['platform == "linux" and target_arch=="arm" and bcm_host==1', {'variables': {'variant': 'raspberry'}}],
  ],
  'targets': [
    {
      'target_name': 'glfw',
      'defines': [
        'VERSION=0.4.6',
      ],
      'include_dirs': [
        "<!(node -e \"require('nan')\")",
        './deps/include',
      ],
      'conditions': [
        ['variant!="raspberry"', {
          'sources': [
            'src/atb.cc',
            'src/glfw.cc'
          ],
        }],
        ['OS=="linux" and variant=="default"', {
          'libraries': [
            '-lAntTweakBar', '<!@(pkg-config --libs glfw3 glew)',
            '-lXrandr','-lXinerama','-lXxf86vm','-lXcursor','-lXi',
            '-lrt','-lm'
            ]
        }],
        ['OS=="linux" and variant=="raspberry"', {
          'library_dirs': ['/opt/vc/lib/'],
          'libraries': ['-lbcm_host', '-lEGL'],
          'include_dirs': ['/opt/vc/include/', '/opt/vc/include/interface/vcos/pthreads', '/opt/vc/include/interface/vmcs_host/linux'],
          'defines': ['__RASPBERRY__'],
          'sources': [
            'src/rpi_shim.cc',
          ],
        }],
        ['OS=="mac"', {
          'include_dirs': [ '<!@(pkg-config glfw3 glew --cflags-only-I | sed s/-I//g)','-I<(ANTTWEAKBAR_ROOT)/include'],
          'libraries': [ '<!@(pkg-config --libs glfw3 glew)', '-L<(ANTTWEAKBAR_ROOT)/lib', '-lAntTweakBar', '-framework OpenGL'],
          'library_dirs': ['/usr/local/lib'],
        }],
        ['OS=="win"', {
            'include_dirs': [
              './deps/include',
              ],
            'library_dirs': [
              './deps/windows/lib/<(target_arch)',
              ],
            'libraries': [
              'FreeImage.lib',
              'AntTweakBar64.lib',
              'glfw3dll.lib',
              'glew32.lib',
              'opengl32.lib'
              ],
            'defines' : [
              'WIN32_LEAN_AND_MEAN',
              'VC_EXTRALEAN'
            ],
            'msvs_settings' : {
              'VCCLCompilerTool' : {
                'AdditionalOptions' : ['/O2','/Oy','/GL','/GF','/Gm-','/EHsc','/MT','/GS','/Gy','/GR-','/Gd']
              },
              'VCLinkerTool' : {
                'AdditionalOptions' : ['/OPT:REF','/OPT:ICF','/LTCG']
              },
            },
            'conditions': [
              ['target_arch=="ia32"', {
                'libraries': ['AntTweakBar.lib']
              }],
              ['target_arch=="x64"', {
                'libraries': ['AntTweakBar64.lib']
              }]
            ]
          },
        ],
      ],
    }
  ]
}
