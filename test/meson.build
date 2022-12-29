euclidean_sources = ['../src/euclidean.c']
lv2_dep = dependency('lv2', required: true)

# Build the SUT
euclideanlib = shared_library('euclideanlib',
                        euclidean_sources,
                        include_directories: inc,
                        dependencies: lv2_dep,
                        install: false)

# Unit tests
test_euclidean_algorithm = executable('test_euclidean', 'test_euclidean_algorithm.c',
                                      include_directories: inc,
                                      link_with: euclideanlib)
test('test the euclidean algorithm implementation', test_euclidean_algorithm)