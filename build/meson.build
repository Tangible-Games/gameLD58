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
        'SDL2_image',
        'GL',
        'jpeg',
        'png',
        'z',
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
    ppsspp_exe = find_program('PPSSPPSDL', '/Applications/PPSSPPSDL.app/Contents/MacOS/PPSSPPSDL')

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
sdl2image_host_dep = dependency('SDL2_image', required: false, native: true)
gtest_dep = dependency('gtest', required: false, native: true)
gtest_main_dep = dependency('gtest_main', required: false, native: true)

gtest_host_dep = [gtest_dep, gtest_main_dep]

python_exe = find_program('python3')

# Meson format
muon_exe = find_program('muon', required: false)
if muon_exe.found()
    meson_files = [
        meson.project_source_root() / 'meson.build',
        meson.project_source_root() / 'build' / 'meson.build',
        meson.project_source_root() / 'symphony_lite' / 'meson.build',
        meson.project_source_root() / 'src' / 'meson.build',
        meson.project_source_root() / 'src' / 'meson.build',
    ]
    run_target('format-meson', command: [muon_exe, 'fmt', '-i'] + meson_files)
endif
