pandoc = find_program('pandoc', required: false)
if pandoc.found()
    man_targets = []
    foreach f: [
        'rofi.1',
        'rofi-sensible-terminal.1',
        'rofi-debugging.5',
        'rofi-dmenu.5',
        'rofi-keys.5',
        'rofi-script.5',
        'rofi-theme-selector.1',
        'rofi-theme.5',
    ]
        section_number = f.split('.')[1]
        install_dest = join_paths(get_option('prefix'), get_option('mandir'), 'man' + section_number)

        man_targets += custom_target(f,
            input: ['.'.join([f, 'markdown']), 'man_filter.lua'],
            output: f,
            command: [ 'pandoc', '--standalone', '--to=man',
                '--lua-filter', '@INPUT1@',
                '-f', 'markdown-tex_math_dollars',
                '@INPUT0@', '-o', '@OUTPUT@' ],
            install: true,
            install_dir: install_dest,
            build_by_default: true,
        )
    endforeach
    run_target('generate-manpage', command: ['true'], depends: man_targets)
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
