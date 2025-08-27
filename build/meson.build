fs = import('fs')

if meson.is_cross_build()
    # PSPDEV configuration and deps
    psp_config = find_program('psp-config')
    pspdev_sdk = run_command(psp_config, '--pspsdk-path', check: true).stdout().strip()

    if pspdev_sdk == ''
        error(
            'PSPDEV is not found. Set the PATH env var to PSPDEV binary forlder',
        )
    endif

    if not fs.is_dir(pspdev_sdk)
        error('Directory `' + pspdev_sdk + '` doesn\'t exist')
    endif

    message('PSPDEV SDK Path: ' + pspdev_sdk)

    pspdev_sdk_inc = include_directories(pspdev_sdk / 'include')

    add_project_arguments('-DSAMPLE_CONFIG_PLATFORM_PSP', language: ['c', 'cpp'])
    add_project_arguments('-D_PSP_FW_VERSION=600', language: ['c', 'cpp'])

    add_project_arguments('-fno-exceptions', language: ['cpp'])
    add_project_arguments('-fno-rtti', language: ['cpp'])

    # Add BUILD_PRX=1 to your makefile to generate a PRX file for use with PSP emulators.
    # To allow your application to use the later PSP models's extra RAM (normally used for UMD caching), add PSP_LARGE_MEMORY=1

    pspdev_link_args = ['-Wl,-zmax-page-size=128']

    pspsdk_lib = pspdev_sdk / 'lib'

    cc = meson.get_compiler('c')

    psp_deps = []
    psp_libs = [
        'pspaudio',
        'pspctrl',
        'pspdebug',
        'pspdisplay',
        'pspge',
        'pspgu',
        'pspgum_vfpu',
        'psphprm',
        'pspnet_apctl',
        'pspnet',
        'psppower',
        'pspvfpu',
        'pspvram',
        # sdl
        'SDL2',
        'SDL2main',
        'GL',
    ]

    foreach lib : psp_libs
        dep = cc.find_library(lib, dirs: pspsdk_lib)
        psp_deps += dep
    endforeach

    pspdev_dep = declare_dependency(
        include_directories: pspdev_sdk_inc,
        dependencies: psp_deps,
    )

    pspdev_deps = [pspdev_dep]

    psp_fix_imports_exe = find_program('psp-fixup-imports')
    mksfoex_exe = find_program('mksfoex')
    strip_exe = find_program('psp-strip')
    pack_pbp_exe = find_program('pack-pbp')
    ppsspp_exe = find_program('PPSSPPSDL')

    add_project_arguments('-std=c++2a', language: 'cpp')
    add_project_arguments('-std=gnu11', language: 'c')
endif

# Host configurations and deps
add_project_arguments('-std=c++2a', language: 'cpp', native: true)
add_project_arguments('-std=gnu11', language: 'c', native: true)

add_project_arguments('-DTARGET_HOST=1', language: ['c', 'cpp'], native: true)
# always debug for the host
add_project_arguments('-O0', language: ['c', 'cpp'], native: true)
add_project_arguments('-ggdb', language: ['c', 'cpp'], native: true)

sdl2_host_dep = dependency('sdl2', required: false, native: true)
gtest_dep = dependency('gtest', required: false, native: true)
gtest_main_dep = dependency('gtest_main', required: false, native: true)

gtest_host_dep = [gtest_dep, gtest_main_dep]

# Generate build targets
foreach t : targets
    srcdir = t['srcdir']
    name = t['name']
    srcs = t['srcs']
    deps = t.get('deps', [])
    icon = t.get('icon', [])
    prev = t.get('preview', [])

    srcs_full = []
    foreach s : srcs
        srcs_full += srcdir / s
    endforeach

    if meson.is_cross_build()
        elf = executable(
            name,
            srcs_full,
            dependencies: pspdev_deps + deps,
        )

        elf_fixed = custom_target(
            name + '_fixed',
            output: name + '-fixed',
            input: elf,
            command: [psp_fix_imports_exe, '@INPUT@', '-o', '@OUTPUT@'],
        )

        param_sfo = custom_target(
            output: name + '-PARAM.SFO',
            command: [mksfoex_exe, '-d', 'MEMSIZE=1', name, '@OUTPUT@'],
        )

        elf_stripped = custom_target(
            input: elf_fixed,
            output: name + '-stripped',
            command: [strip_exe, '@INPUT@', '-o', '@OUTPUT@'],
        )

        eboot_pbp = custom_target(
            input: [param_sfo, elf_stripped, icon, prev],
            output: name + '-EBOOT.PBP',
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

        run_target(
            name + '_run',
            command: [ppsspp_exe, '--escape-exit', eboot_pbp.full_path()],
            depends: eboot_pbp,
        )
    endif

    if sdl2_host_dep.found()
        host_elf = executable(
            name + '_host_exe',
            srcs_full,
            dependencies: [sdl2_host_dep] + deps,
            native: true,
        )

        run_target(name + '_host_run', command: [host_elf], depends: host_elf)
    endif
endforeach

# Meson format

muon_exe = find_program('muon')
if muon_exe.found()
    meson_files = [
        meson.project_source_root() / 'meson.build',
        meson.project_source_root() / 'build' / 'meson.build',
        meson.project_source_root() / 'symphony_lite' / 'meson.build',
    ]
    run_target('format-meson', command: [muon_exe, 'fmt', '-i'] + meson_files)
endif

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
