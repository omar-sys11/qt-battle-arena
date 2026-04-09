cmake_minimum_required(VERSION 3.19)
project(CS2-Battle-Arena LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia)
qt_standard_project_setup()

qt_add_executable(CS2-Battle-Arena
    WIN32 MACOSX_BUNDLE

    # ── Entry point ────────────────────────────
    src/main.cpp

    # ── Pure C++ logic (no Qt) ─────────────────
    include/character.h         src/character.cpp
    include/warrior.h           src/warrior.cpp
    include/mage.h              src/mage.cpp
    include/archer.h            src/archer.cpp
    include/battleresult.h
    include/scoretracker.h      src/scoretracker.cpp

    # ── Game engine (Qt Core) ──────────────────
    include/gameengine.h        src/gameengine.cpp

    # ── Main window shell ──────────────────────
    include/mainwindow.h        src/mainwindow.cpp
    src/mainwindow.ui

    # ── Screens ────────────────────────────────
    include/startscreenwidget.h         src/startscreenwidget.cpp
    include/characterselectiondialog.h  src/characterselectiondialog.cpp
    src/characterselectiondialog.ui
    include/gameoverwidget.h            src/gameoverwidget.cpp
    include/scoreboardwidget.h          src/scoreboardwidget.cpp

    # ── Battle scene widgets ───────────────────
    include/battlewidget.h      src/battlewidget.cpp
    include/battlemenuwidget.h  src/battlemenuwidget.cpp
    include/battlelogwidget.h   src/battlelogwidget.cpp
    include/spritewidget.h      src/spritewidget.cpp
    include/healthbarwidget.h   src/healthbarwidget.cpp
    include/pauseoverlaywidget.h src/pauseoverlaywidget.cpp
    include/roundoverwidget.h   src/roundoverwidget.cpp
    include/audiomanager.h
    src/audiomanager.cpp
)

# Tell the compiler where your headers live
target_include_directories(CS2-Battle-Arena PRIVATE include)

# Resources (font, stylesheet, sprites, backgrounds)
qt_add_resources(CS2-Battle-Arena "assets"
    PREFIX "/"
   BASE resources
    FILES
        resources/fonts/PressStart2P-Regular.ttf
        resources/styles/game.qss
        resources/sprites/warrior_front.png
        resources/sprites/warrior_back.png
        resources/sprites/mage_front.png
        resources/sprites/mage_back.png
        resources/sprites/archer_front.png
        resources/sprites/archer_back.png
        resources/backgrounds/battle_bg.png
        resources/music/battle.ogg
        resources/music/menu.ogg
        resources/sfx/hit.wav
        resources/sfx/faint.wav
        resources/sfx/special.wav
)

target_link_libraries(CS2-Battle-Arena
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
)

include(GNUInstallDirs)
install(TARGETS CS2-Battle-Arena
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
qt_generate_deploy_app_script(
    TARGET CS2-Battle-Arena
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
