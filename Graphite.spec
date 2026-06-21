# -*- mode: python ; coding: utf-8 -*-

block_cipher = None

a = Analysis(
    ['main.py'],  # Your main entry point
    pathex=[],
    binaries=[],
    datas=[
        ('public/', 'public/'),               # Bundles your SVG icons directory
        ('audio_settings.json', '.'),         # Default settings configuration file
        ('logo.png', '.'),                     # Your PNG logo
        ('splashscreen.png', '.'),             # Your splash screen image
    ],
    hiddenimports=[
        'PySide6.QtCore',
        'PySide6.QtWidgets',
        'PySide6.QtGui',
        'PySide6.QtSvg',                      # Required for parsing SVG icons
    ],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=['test_daw.py'],                 # Excludes test suite from the build
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='Graphite',                          # Final application name
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=False,                            # Hides the Windows command prompt window
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements=None,
    icon='logo.ico',                          # Uses your native Windows icon file
)

coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='Graphite',
)