project('spt', 'cpp')
add_project_arguments(['-DPOSIX', '-DGNU', '-DGNUC', '-D_LINUX', '-w'], language: 'cpp')
incdir = include_directories(['spt/utils', '.', 'SDK/steampipe/public', 'SDK/steampipe/public/tier0', 'SDK/steampipe/public/tier1', 'SDK/steampipe/common',
'SDK/steampipe', 'SDK/steampipe/game/shared', 'SDK/steampipe/game/server', 'SDK/steampipe/game/client'])
sources = [
    'spt/feature.cpp',
    'spt/cvars.cpp',
    'spt/features/autojump.cpp',
    'spt/features/hud.cpp',
    'spt/features/nosleep.cpp',
    'spt/features/pause.cpp',
    'spt/features/taslogging.cpp',
    'spt/spt-serverplugin.cpp',
    'spt/sptlib-wrapper.cpp',
    'spt/utils/math.cpp',
    'spt/utils/signals.cpp',
    'spt/utils/string_utils.cpp',
    'SPTLib/Hooks.cpp',
    'SPTLib/IHookableDirFilter.cpp',
    'SPTLib/IHookableModule.cpp',
    'SPTLib/IHookableNameFilter.cpp',
    'SPTLib/IHookableNameFilterOrdered.cpp',
    'SPTLib/MemUtils.cpp',
    'SPTLib/sptlib-stdafx.cpp',
    'SPTLib/sptlib.cpp',
    'SPTLib/Linux/Hooks_linux.cpp',
    'SPTLib/Linux/MemUtils_linux.cpp'
]

cmplr = meson.get_compiler('cpp')
libdirs = [meson.current_source_dir() + '/SDK/steampipe/lib/public/linux32/']
tier0 = cmplr.find_library('tier0', dirs : libdirs)
tier1 = cmplr.find_library('tier1', static : true, dirs : libdirs)
tier3 = cmplr.find_library('tier3', static : true, dirs : libdirs)
mathlib = cmplr.find_library('mathlib', static : true, dirs : libdirs)

library('spt', sources, name_prefix: '', include_directories : incdir, dependencies : [tier0, tier1, tier3, mathlib])