project('euclidean.lv2','c', 'cpp',
    version: '0.2.0-SNAPSHOT',
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
