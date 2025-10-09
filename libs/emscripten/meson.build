if meson.is_cross_build() and host_machine.system() == 'emscripten'
    # Use prebuilt library and copy of original headers

    cc = meson.get_compiler('c')

    prebuilt_inc_dir = include_directories('includes')
    prebuilt_lib_dir = [meson.current_source_dir()]

    emscripten_sdl3_dep = declare_dependency(
        include_directories: prebuilt_inc_dir,
        dependencies: [
            cc.find_library('SDL3', dirs: prebuilt_lib_dir),
            cc.find_library('SDL3_image', dirs: prebuilt_lib_dir),
        ],
    )

    emscripten_deps = [emscripten_sdl3_dep]

endif
