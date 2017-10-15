project('rofi', 'c',
    version: '1.4.2',
    meson_version: '>=0.39.1',
    license: [ 'MIT' ],
    default_options: [
        'c_std=c99',
        'warning_level=3',
    ],
)

c_compiler = meson.get_compiler('c')

add_project_arguments(
    '-I@0@'.format(meson.build_root()),
    '-I@0@'.format(join_paths(meson.source_root(), 'include')),
    language: 'c'
)

flags = [
    '-Wparentheses',
    '-Winline',
    '-Wunreachable-code',
    '-Werror=missing-prototypes',
]
if get_option('enable-asan')
    flags += [
        '-fsanitize=address',
        '-fno-omit-frame-pointer',
    ]
endif
foreach f : flags
    if c_compiler.has_argument(f)
        add_project_arguments(f, language: 'c')
    endif
endforeach

plugindir = join_paths(get_option('prefix'), get_option('libdir'), meson.project_name())
themedir = join_paths(get_option('prefix'), get_option('datadir'), meson.project_name(), 'themes')

deps = [
    dependency('glib-2.0', version: '>= 2.40'),
    dependency('gio-unix-2.0'),
    dependency('gmodule-2.0'),
    dependency('cairo'),
    dependency('pango'),
    dependency('pangocairo'),
    dependency('xkbcommon'),
    dependency('librsvg-2.0'),
    c_compiler.find_library('m', required: false),
]

# XCB stuff
deps += [
    subproject('libgwater/xcb').get_variable('libgwater_xcb'),
    dependency('xcb-aux'),
    dependency('xcb-xkb'),
    dependency('xkbcommon-x11'),
    dependency('xcb-ewmh'),
    dependency('xcb-icccm'),
    dependency('xcb-xrm'),
    dependency('xcb-randr'),
    dependency('xcb-xinerama'),
    dependency('cairo-xcb'),
    dependency('libstartup-notification-1.0'),
]

check = dependency('check', version: '>= 0.11.0', required: get_option('enable-check'))

header_conf = configuration_data()
header_conf.set_quoted('PACKAGE_NAME', meson.project_name())
header_conf.set_quoted('PACKAGE_VERSION', meson.project_version())
header_conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
header_conf.set_quoted('PACKAGE_BUGREPORT', 'https://github.com/DaveDavenport/rofi/')
header_conf.set_quoted('PACKAGE_URL', 'https://reddit.com/r/qtools/')

header_conf.set('THEME_CONVERTER', true)
header_conf.set('_GNU_SOURCE', true)

git = find_program('git', required: false)
if git.found()
	git_version = run_command(git, '--git-dir', join_paths(meson.source_root(), '.git'), 'describe', '--tags', '--always', '--dirty')
	git_branch = run_command(git, '--git-dir', join_paths(meson.source_root(), '.git'), 'describe', '--tags', '--always', '--all')
	if git_version.returncode() == 0 and git_branch.returncode() == 0
		git_branch_parts = []
		foreach b : git_branch.stdout().strip().split('/')
			if b != 'heads'
				git_branch_parts += b
			endif
		endforeach
		header_conf.set_quoted('GIT_VERSION', '@0@ - @1@ (@2@)'.format(meson.project_version(), git_version.stdout().strip(), '/'.join(git_branch_parts)))
	endif
endif

header_conf.set('ENABLE_DRUN', get_option('enable-drun'))
header_conf.set('WINDOW_MODE', get_option('enable-window'))
header_conf.set('TIMINGS', get_option('enable-timings'))

header_conf.set_quoted('MANPAGE_PATH', join_paths(get_option('prefix'), get_option('mandir')))
header_conf.set_quoted('SYSCONFDIR', join_paths(get_option('prefix'), get_option('sysconfdir')))
header_conf.set_quoted('PLUGIN_PATH', plugindir)
header_conf.set_quoted('THEME_DIR', themedir)

config_h = configure_file(output: 'config.h', configuration: header_conf)

nk_modules = [
    'bindings=true',
    'xdg-theme=true',
]
nk = subproject('libnkutils', default_options: nk_modules)
nk_options = nk.get_variable('nk_options')
foreach o : nk_modules + nk_options
    if not nk_modules.contains(o) or not nk_options.contains(o)
        error('You must not change libnkutils options @0@ != @1@'.format('|'.join(nk_modules), '|'.join(nk_options)))
    endif
endforeach
deps += nk.get_variable('libnkutils')

install_headers([
        'include/mode.h',
        'include/mode-private.h',
        'include/helper.h',
    ],
    subdir: meson.project_name(),
)
install_data(
    'script/rofi-sensible-terminal',
    'script/rofi-theme-selector',
    install_dir: join_paths(get_option('prefix'), get_option('bindir'))
)

flex = generator(find_program('flex'),
    output: '@BASENAME@.c',
    arguments: [ '-o', '@OUTPUT@', '@INPUT@' ]
)
bison = generator(find_program('bison'),
    output: [ '@BASENAME@.c', '@BASENAME@.h' ],
    arguments: [ '-y', '@INPUT@', '--defines=@OUTPUT1@', '--output=@OUTPUT0@' ]
)

rofi_sources = files(
        'source/rofi.c',
        'source/view.c',
        'source/mode.c',
        'source/keyb.c',
        'config/config.c',
        'source/helper.c',
        'source/timings.c',
        'source/history.c',
        'source/theme.c',
        'source/css-colors.c',
        'source/widgets/box.c',
        'source/widgets/container.c',
        'source/widgets/widget.c',
        'source/widgets/textbox.c',
        'source/widgets/listview.c',
        'source/widgets/scrollbar.c',
        'source/xrmoptions.c',
        'source/rofi-types.c',
        'source/xcb.c',
        'source/dialogs/run.c',
        'source/dialogs/ssh.c',
        'source/dialogs/drun.c',
        'source/dialogs/dmenu.c',
        'source/dialogs/combi.c',
        'source/dialogs/window.c',
        'source/dialogs/script.c',
        'source/dialogs/help-keys.c',
        'include/display.h',
        'include/xcb.h',
        'include/xcb-internal.h',
        'include/rofi.h',
        'include/mode.h',
        'include/mode-private.h',
        'include/settings.h',
        'include/keyb.h',
        'include/view.h',
        'include/view-internal.h',
        'include/helper.h',
        'include/helper-theme.h',
        'include/timings.h',
        'include/history.h',
        'include/theme.h',
        'include/rofi-types.h',
        'include/css-colors.h',
        'include/default-theme.h',
        'include/widgets/box.h',
        'include/widgets/container.h',
        'include/widgets/widget.h',
        'include/widgets/widget-internal.h',
        'include/widgets/textbox.h',
        'include/widgets/listview.h',
        'include/widgets/scrollbar.h',
        'include/xrmoptions.h',
        'include/dialogs/ssh.h',
        'include/dialogs/run.h',
        'include/dialogs/drun.h',
        'include/dialogs/dmenu.h',
        'include/dialogs/combi.h',
        'include/dialogs/script.h',
        'include/dialogs/window.h',
        'include/dialogs/dialogs.h',
        'include/dialogs/help-keys.h',
)
theme_lexer_sources = files('lexer/theme-lexer.l')
theme_parser_sources = files('lexer/theme-parser.y')

theme_lexer = flex.process(theme_lexer_sources)
theme_parser = bison.process(theme_parser_sources)
rofi = executable('rofi', rofi_sources + [
        theme_lexer,
        theme_parser,
    ],
    dependencies: deps,
    install: true,
)

subdir('doc')
install_man(
    'doc/rofi.1',
    'doc/rofi-sensible-terminal.1',
    'doc/rofi-theme.5',
)

install_data(
    'themes/Adapta-Nokto.rasi',
    'themes/Arc.rasi',
    'themes/DarkBlue.rasi',
    'themes/Pop-Dark.rasi',
    'themes/Indego.rasi',
    'themes/Monokai.rasi',
    'themes/Paper.rasi',
    'themes/android_notification.rasi',
    'themes/arthur.rasi',
    'themes/blue.rasi',
    'themes/c64.rasi',
    'themes/dmenu.rasi',
    'themes/glue_pro_blue.rasi',
    'themes/gruvbox-dark-hard.rasi',
    'themes/gruvbox-dark-soft.rasi',
    'themes/gruvbox-dark.rasi',
    'themes/gruvbox-light-hard.rasi',
    'themes/gruvbox-light-soft.rasi',
    'themes/gruvbox-light.rasi',
    'themes/lb.rasi',
    'themes/paper-float.rasi',
    'themes/purple.rasi',
    'themes/sidebar.rasi',
    'themes/solarized.rasi',
    'themes/solarized_alternate.rasi',
    install_dir: themedir
)

pkgconfig = import('pkgconfig')
pkgconfig.generate(
    name: 'rofi',
    filebase: 'rofi',
    description: 'Header files for rofi plugins',
    version: meson.project_version(),
    requires_private: [
        'glib-2.0 >= 2.40',
        'gmodule-2.0',
        'cairo',
    ],
)


test('history test', executable('history.test', [
        'test/history-test.c',
    ],
    objects: rofi.extract_objects([
        'source/history.c',
        'config/config.c',
    ]),
    dependencies: deps,
))

test('helper_pidfile test', executable('helper_pidfile.test', [
        'test/helper-pidfile.c',
    ],
    objects: rofi.extract_objects([
        'config/config.c',
        'source/helper.c',
        'source/xrmoptions.c',
        'source/rofi-types.c',
    ]),
    dependencies: deps,
))

test('helper_tokenize test', executable('helper_tokenize.test', [
        'test/helper-tokenize.c',
    ],
    objects: rofi.extract_objects([
        'config/config.c',
        'source/helper.c',
        'source/xrmoptions.c',
        'source/rofi-types.c',
    ]),
    dependencies: deps,
))

test('widget test', executable('widget.test', [
        'test/widget-test.c',
        theme_parser,
        theme_lexer,
    ],
    objects: rofi.extract_objects([
        'source/widgets/widget.c',
        'source/widgets/textbox.c',
        'source/theme.c',
        'source/rofi-types.c',
        'source/css-colors.c',
        'source/helper.c',
        'config/config.c',
    ]),
    dependencies: deps,
))

test('box test', executable('box.test', [
        'test/box-test.c',
        theme_parser,
        theme_lexer,
    ],
    objects: rofi.extract_objects([
        'source/widgets/widget.c',
        'source/widgets/box.c',
        'source/theme.c',
        'source/rofi-types.c',
        'source/css-colors.c',
        'config/config.c',
    ]),
    dependencies: deps,
))

test('scrollbar test', executable('scrollbar.test', [
        'test/scrollbar-test.c',
        theme_parser,
        theme_lexer,
    ],
    objects: rofi.extract_objects([
        'source/widgets/widget.c',
        'source/widgets/scrollbar.c',
        'source/theme.c',
        'source/rofi-types.c',
        'source/css-colors.c',
        'config/config.c',
    ]),
    dependencies: deps,
))

test('textbox test', executable('textbox.test', [
        'test/textbox-test.c',
        theme_parser,
        theme_lexer,
    ],
    objects: rofi.extract_objects([
        'source/widgets/widget.c',
        'source/widgets/textbox.c',
        'source/theme.c',
        'source/rofi-types.c',
        'source/css-colors.c',
        'source/helper.c',
        'config/config.c',
    ]),
    dependencies: deps,
))

test('helper test', executable('helper.test', [
        'test/helper-test.c',
    ],
    objects: rofi.extract_objects([
        'config/config.c',
        'source/helper.c',
        'source/xrmoptions.c',
        'source/rofi-types.c',
    ]),
    dependencies: deps,
))

test('helper_expand test', executable('helper_expand.test', [
        'test/helper-expand.c',
    ],
    objects: rofi.extract_objects([
        'config/config.c',
        'source/helper.c',
        'source/xrmoptions.c',
        'source/rofi-types.c',
    ]),
    dependencies: deps,
))

test('helper_config_cmdline_parser test', executable('helper_config_cmdline_parser.test', [
        'test/helper-config-cmdline-parser.c',
    ],
    objects: rofi.extract_objects([
        'config/config.c',
        'source/helper.c',
        'source/xrmoptions.c',
        'source/rofi-types.c',
    ]),
    dependencies: deps,
))

if check.found()
    deps+= [ check ]

    test('theme_parser test', executable('theme_parser.test', [
            'test/theme-parser-test.c',
            theme_lexer,
            theme_parser,
        ],
        objects: rofi.extract_objects([
            'config/config.c',
            'source/helper.c',
            'source/xrmoptions.c',
            'source/theme.c',
            'source/rofi-types.c',
            'source/css-colors.c',
        ]),
        dependencies: deps,
    ))

    test('mode test', executable('mode.test', [
            'test/mode-test.c',
        ],
        objects: rofi.extract_objects([
            'config/config.c',
            'source/dialogs/help-keys.c',
            'source/helper.c',
            'source/mode.c',
            'source/xrmoptions.c',
            'source/rofi-types.c',
            'source/keyb.c',
        ]),
        dependencies: deps,
    ))
endif


run_target('test-x', command: [ 'test/run_all_tests.sh' ], depends: [ rofi ])

uncrustify = find_program('uncrustify', required: false)
if uncrustify.found()
    run_target('indent',
        command: [
            uncrustify,
            '-c', join_paths(meson.source_root(), 'data', 'uncrustify.cfg'),
            '--replace',
            rofi_sources
        ],
    )
endif

rofi_sources += theme_lexer_sources
rofi_sources += theme_parser_sources

cppcheck = find_program('cppcheck', required: false)
if cppcheck.found()
    run_target('cppcheck',
        command: [
            cppcheck,
            '--std=@0@'.format(get_option('c_std')),
            '--platform=unix64',
            '--enable=all',
            '-Uerror_dialog',
            '--inconclusive',
            '-I@0@'.format(join_paths(meson.source_root(), 'include')),
            rofi_sources
        ],
    )
endif

ohcount = find_program('ohcount', required: false)
if ohcount.found()
    run_target('ohcount',
        command: [
            ohcount,
            rofi_sources
        ],
    )
endif
