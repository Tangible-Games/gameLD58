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

srcs = []
include_dirs = []

subdir('libs')
subdir('src')

# Build configuration and dependencies
subdir('build')

nlohmann_json = subproject('nlohmann_json')
nlohmann_json_dep = nlohmann_json.get_variable('nlohmann_json_dep')

common_deps = [symphony_lite_dep, vlog_dep, nlohmann_json_dep]

icon = files('icon.png')
preview = files('icon.png')

assets_link = custom_target(
    'assets_link',
    output: '.empty',
    command: [
        python_exe,
        [
            meson.project_source_root() / 'build' / 'link_assets.py',
            meson.project_source_root() / 'assets',
            meson.project_build_root(),
        ],
    ],
    build_always_stale: true,
)

# Generate build targets
if meson.is_cross_build()
    elf = executable(
        meson.project_name(),
        srcs,
        include_directories: include_dirs,
        dependencies: pspdev_deps + common_deps,
    )

    elf_fixed = custom_target(
        meson.project_name() + '_fixed',
        output: meson.project_name() + '-fixed',
        input: elf,
        command: [psp_fix_imports_exe, '@INPUT@', '-o', '@OUTPUT@'],
    )

    param_sfo = custom_target(
        output: meson.project_name() + '-PARAM.SFO',
        command: [mksfoex_exe, '-d', 'MEMSIZE=1', meson.project_name(), '@OUTPUT@'],
    )

    elf_stripped = custom_target(
        input: elf_fixed,
        output: meson.project_name() + '-stripped',
        command: [strip_exe, '@INPUT@', '-o', '@OUTPUT@'],
    )

    eboot_pbp = custom_target(
        input: [param_sfo, elf_stripped, icon, preview],
        output: meson.project_name() + '-EBOOT.PBP',
        command: [
            pack_pbp_exe,
            '@OUTPUT@',
            '@INPUT0@',
            '@INPUT2@',
            'NULL',
            '@INPUT3@',
            'NULL',
            'NULL',
            '@INPUT1@',
            'NULL',
        ],
    )

    if ppsspp_exe.found()
        run_target(
            'run',
            command: [ppsspp_exe, '--escape-exit', eboot_pbp.full_path()],
            depends: [eboot_pbp, assets_link],
        )
    endif

    game_zip = custom_target(
        input: eboot_pbp,
        output: meson.project_name() + '.zip',
        command: [
            python_exe,
            meson.project_source_root() / 'build' / 'build_zip.py',
            '@OUTPUT@',
            meson.project_build_root(),
            '@INPUT@',
            meson.project_source_root() / 'assets',
        ],
    )
endif

if sdl2_host_dep.found()
    host_elf = executable(
        meson.project_name() + '_host_exe',
        srcs,
        include_directories: include_dirs,
        dependencies: [sdl2_host_dep, sdl2image_host_dep] + common_deps,
        native: true,
    )

    run_target('host_run', command: [host_elf], depends: [host_elf, assets_link])
endif

# Tests
if gtest_dep.found()
    foreach t : tests_srcs
        test_name = fs.replace_suffix(fs.name(t), '')
        message('Test: ' + test_name)
        texe = executable(
            test_name,
            t,
            include_directories: include_directories('.'),
            dependencies: [gtest_host_dep],
            native: true,
        )
        test(test_name, texe)
    endforeach
endif
