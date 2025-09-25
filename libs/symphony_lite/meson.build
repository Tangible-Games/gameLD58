symphony_lite_dep = declare_dependency(include_directories: '..', dependencies: [])

if not get_option('logging')
    add_project_arguments('-DVLOG_ENABLED=0', language: ['c', 'cpp'])
    add_project_arguments('-DVLOG_ENABLED=0', language: ['c', 'cpp'], native: true)
endif

tests_srcs = files(
    'aa_rect2d_test.cpp',
    'formatted_text_test.cpp',
    'measured_text_test.cpp',
    'point2d_test.cpp',
    'ray_casting_projection_test.cpp',
    'segment2d_test.cpp',
    'transformation_matrix3d_test.cpp',
    'utf8_test.cpp',
    'vector2d_test.cpp',
)
