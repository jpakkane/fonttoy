project('fonttoy', 'cpp',
    license: 'GPLv2+',
    default_options: ['cpp_std=c++17', 'warning_level=3', 'default_library=static'])

lbfgs_dep = dependency('liblbfgs', fallback: ['liblbfgs', 'liblbfgs_dep'])
tinyxml2_dep = dependency('tinyxml2', fallback: ['tinyxml2', 'tinyxml2_dep'])

if host_machine.cpu().startswith('wasm')
    add_project_arguments('-DWASM', language: 'cpp')
    configure_file(input: 'index.html',
        output: 'index.html',
        copy: true)
    install_data('index.html', install_dir: get_option('bindir'))
endif

l = static_library('flib', 'fonttoy.cpp', 'constraints.cpp', 'parser.cpp')

executable('fonttoy', 'main.cpp', 'svgexporter.cpp', 'svgexporter.cpp',
    link_with: l,
    install: true,
    dependencies: [tinyxml2_dep, lbfgs_dep])

executable('parsertest', 'parsertest.cpp', link_with: l)
