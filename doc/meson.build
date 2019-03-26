ronn = find_program('ronn', required: false)
if ronn.found()
    run_target('update-manpage',
        command: [ ronn, '--roff', files(
            'rofi.1.markdown',
            'rofi-theme-selector.1.markdown',
            'rofi-theme.5.markdown',
        )]
    )
endif

doxy_conf = configuration_data()
doxy_conf.set('PACKAGE', meson.project_name())
doxy_conf.set('VERSION', meson.project_version())
doxy_conf.set('abs_builddir', join_paths(meson.build_root(), meson.current_build_dir()))
doxy_conf.set('abs_top_srcdir', meson.source_root())

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
