project('fonttoy', 'cpp',
    license: 'GPLv2+',
    default_options: ['cpp_std=c++17', 'warning_level=3'])

lbfgs_dep = dependency('liblbfgs', fallback: ['liblibfgs', 'liblbfgs_dep'])

executable('fonttoy', 'main.cpp', 'fonttoy.cpp',
    dependencies: lbfgs_dep)
