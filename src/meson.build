euclidean_sources = ['euclidean.c']
euclideanlib = shared_library('euclidean',
                        euclidean_sources,
                        include_directories : inc,
                        dependencies : lv2_dep,
                        install : true)

# Install folder
lv2_directory = join_paths(get_option('libdir'), 'lv2')
install_folder = join_paths(lv2_directory, meson.project_name())

# Sources to compile
euclidean_src = ['plugins/plugin.c', 'euclidean.c']

# Dependencies for euclidean rhythms
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

# Configure euclidean.ttl
euclidean_ttl = configure_file(
    input: join_paths('lv2ttl', 'euclidean.ttl.in'),
    output: 'euclidean.ttl',
    configuration: data_conf,
    install: true,
	install_dir: install_folder
)
