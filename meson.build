project(
    'gameLD58',
    'c',
    'cpp',
    version: '0.1',
    meson_version: '>= 0.60',
    default_options: [
        'warning_level=3',
        'werror=true',
        'default_library=static',
        'buildtype=plain',
        'debug=false',
        'optimization=plain',
    ],
)

subdir('symphony_lite')

# targets
game_elf = {
    'name': meson.project_name(),
    'srcs': [
        'main.cpp',
    ],
    'deps': [symphony_lite_dep],
    'icon': [meson.current_source_dir() / 'icon.png'],
    'preview': [meson.current_source_dir() / 'prev.png'],
    'srcdir': meson.current_source_dir(),
}

targets = [game_elf]

# build scripts (include at the end)
subdir('build')
