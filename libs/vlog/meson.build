vlog_dep = declare_dependency(include_directories: '..', dependencies: [])

if not get_option('logging')
    add_project_arguments('-DVLOG_ENABLED=0', language: ['c', 'cpp'])
    add_project_arguments('-DVLOG_ENABLED=0', language: ['c', 'cpp'], native: true)
endif
