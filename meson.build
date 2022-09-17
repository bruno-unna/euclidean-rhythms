project('euclidean-rhythms.lv2','c',version: '0.1.0-SNAPSHOT',default_options: ['default_library=shared','c_std=c99'])

# Install folder
lv2_directory = join_paths(get_option('libdir'), 'lv2')
install_folder = join_paths(lv2_directory, meson.project_name())

# Sources to compile
euclidean_rhythms_src = ['plugins/eurhythms.c', 'src/euclidean_algorithms.c']

# Dependencies for euclidean rhythms
lv2_dep = dependency('lv2', required: true)
m_dep = meson.get_compiler('c').find_library('m', required: true)
all_dep = [lv2_dep,m_dep]

# Get the host operating system and cpu architecture
current_os = host_machine.system()
current_arch = build_machine.cpu_family()

# Shared c_args for libraries
lib_c_args = ['-fvisibility=hidden']

# Add default x86 and x86_64 optimizations
if current_arch == 'x86' or current_arch == 'x86_64' and current_os != 'darwin'
    lib_c_args += ['-msse','-msse2','-mfpmath=sse','-ffast-math','-fomit-frame-pointer','-fno-finite-math-only']
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

# Build of the shared object
library('eurhythms',
    euclidean_rhythms_src,
    c_args: lib_c_args,
    name_prefix: '',
    dependencies: all_dep,
    install: true,
    install_dir: install_folder
)

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

data_conf = configuration_data()
data_conf.set('MAJOR_VERSION', version_array[0])
data_conf.set('MINOR_VERSION', version_array[1])
data_conf.set('MICRO_VERSION', version_array[2])

# Configure eurhythms.ttl
eurhythms_ttl = configure_file(
    input: join_paths('lv2ttl', 'eurhythms.ttl.in'),
    output: 'eurhythms.ttl',
    configuration: data_conf,
    install: true,
	install_dir: install_folder
)
