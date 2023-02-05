project('euclidean.lv2','c',
    version: '0.2.0-SNAPSHOT',
    default_options: ['default_library=shared','c_std=c99'])

# Where to install?
lv2_directory = join_paths(get_option('libdir'), 'lv2')
install_folder = join_paths(lv2_directory, meson.project_name())

# Where are the includes?
inc = include_directories('include')

subdir('include')
subdir('src')
subdir('test')

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : euclideanmodule,
                 version : '0.2.0-SNAPSHOT',
                 name : 'euclidean',
                 filebase : 'euclidean',
                 description : 'A plugin to produce euclidean rhythms.')
