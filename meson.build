project('euclidean.lv2','c',
    version: '0.1.2',
    default_options: ['default_library=shared','c_std=c99'])

# Where to install?
libdir = get_option('libdir')
if libdir.endswith('.lv2') or libdir.endswith('.lv2/')
    install_folder = join_paths(libdir, meson.project_name())
else
    lv2_directory = join_paths(libdir, 'lv2')
    install_folder = join_paths(lv2_directory, meson.project_name())
endif

# Where are the includes?
inc = include_directories('include')

subdir('src')
subdir('test')

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : euclideanmodule,
                 version : '0.1.2',
                 name : 'euclidean',
                 filebase : 'euclidean',
                 description : 'A plugin to produce euclidean rhythms.')
