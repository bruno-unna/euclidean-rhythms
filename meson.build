project('euclidean.lv2','c',
    version: '0.1.0-SNAPSHOT',
    default_options: ['default_library=shared','c_std=c99'])

lv2_dep = dependency('lv2', required: true)

inc = include_directories('include')

subdir('include')
subdir('src')
subdir('test')

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : euclideanlib,
                 version : '0.1',
                 name : 'libeuclidean',
                 filebase : 'euclidean',
                 description : 'A plugin to produce euclidean rhythms.')
