gomd2man = find_program('go-md2man', required: false)
if gomd2man.found()
    man_targets = []
    foreach f: [
        'rofi.1',
        'rofi-theme-selector.1',
        'rofi-theme.5',
        'rofi-dmenu.5',
        'rofi-debugging.5',
        'rofi-keys.5',
        'rofi-script.5',
        'rofi-sensible-terminal.1'
    ]
        man_targets += custom_target(f,
            input: '.'.join([f, 'markdown']),
            output: f,
            command: [ 'go-md2man',
                '-in', files('.'.join([f,'markdown'])),
                '-out', files(f)
            ])
    endforeach
    run_target('update-manpage', command: ['true'], depends: man_targets)
endif

doxy_conf = configuration_data()
doxy_conf.set('PACKAGE', meson.project_name())
doxy_conf.set('VERSION', meson.project_version())
doxy_conf.set('abs_builddir', join_paths(meson.project_build_root(), meson.current_build_dir()))
doxy_conf.set('abs_top_srcdir', meson.project_source_root())

doxyfile = configure_file(
    input: 'rofi.doxy.in',
    output: 'rofi.doxy',
    configuration: doxy_conf,
)

doxygen = find_program('doxygen', required: false)
if doxygen.found()
    html_target = custom_target('doxy',
        input: doxyfile,
        output: 'html',
        command: [doxygen, doxyfile],
        install: false,
    )
endif
