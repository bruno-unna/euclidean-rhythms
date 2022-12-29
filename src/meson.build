# Get the host operating system and cpu architecture
current_os = host_machine.system()
current_arch = build_machine.cpu_family()

# Add default x86 and x86_64 optimizations
if current_arch == 'x86' or current_arch == 'x86_64' and current_os != 'darwin'
    lib_c_args = ['-msse','-msse2','-mfpmath=sse','-ffast-math','-fomit-frame-pointer','-fno-finite-math-only']
endif

# Configure extension for shared object
if current_os == 'darwin' #mac
    extension = '.dylib'
else #unix like (linux, bsd)   
    extension = '.so'
endif 
if current_os == 'windows' #windows
    extension = '.dll'
endif

# Dependencies for euclidean rhythms
lv2_dep = dependency('lv2', required: true)
m_dep = meson.get_compiler('c').find_library('m', required: true)
all_dep = [lv2_dep, m_dep]

# Sources to compile
euclidean_sources = ['euclidean.c', 'plugins/plugin.c']

# Definition of the actual module
euclideanlib = shared_module('euclidean',
                        euclidean_sources,
                        include_directories: inc,
                        c_args: lib_c_args,
                        name_prefix: '',
                        dependencies: all_dep,
                        install: true,
                        install_dir: install_folder)

# Getting version from project configuration or from git tags
version_array = meson.project_version().split('.')

git = find_program('git', native: true, required: false)
if version_array.length() == 0 and git.found()
    version_array = run_command('git', 'describe').stdout().strip().split('-')[0].split('.')
endif

# Configure manifest.ttl
manifest_conf = configuration_data()
manifest_conf.set('LIB_EXT', extension)
manifest_ttl = configure_file(
    input: 'lv2ttl/manifest.ttl.in',
    output: 'manifest.ttl',
    configuration: manifest_conf,
    install: true,
	install_dir: install_folder
)

# Configure euclidean.ttl
data_conf = configuration_data()
data_conf.set('MAJOR_VERSION', version_array[0])
data_conf.set('MINOR_VERSION', version_array[1])
data_conf.set('MICRO_VERSION', version_array[2])
euclidean_ttl = configure_file(
    input: join_paths('lv2ttl', 'euclidean.ttl.in'),
    output: 'euclidean.ttl',
    configuration: data_conf,
    install: true,
	install_dir: install_folder
)
