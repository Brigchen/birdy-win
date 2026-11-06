cmake_minimum_required(VERSION 3.16)
project(BirdyQt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets)

add_executable(birdy
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/ui/Cards.cpp
    src/ui/SettingsForm.cpp
    src/ui/StatusPanel.cpp
    src/infer/BirdEngine.h
    src/infer/BirdEngine.cpp
)

target_include_directories(birdy PRIVATE src)
target_link_libraries(birdy PRIVATE Qt5::Widgets)

if(WIN32)
    set_target_properties(birdy PROPERTIES WIN32_EXECUTABLE TRUE)

    # Anaconda 的 Qt 库叫 Qt5Core_conda.dll，双击 exe 时不会去 Anaconda 目录里找。
    # 编完后把 Qt 和它依赖的 zlib / zstd / libpng / OpenSSL 放到 exe 旁边。
    get_target_property(_qmake_exe Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qmake_exe}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin}")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET birdy POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --$<IF:$<CONFIG:Debug>,debug,release>
                    --no-translations
                    --no-compiler-runtime
                    --no-opengl-sw
                    "$<TARGET_FILE:birdy>"
            COMMENT "Deploy Qt libraries next to birdy.exe"
            VERBATIM
        )
        foreach(_dep IN ITEMS zlib.dll zstd.dll libpng16.dll libjpeg.dll libssl-3-x64.dll libcrypto-3-x64.dll Qt5Pdf_conda.dll)
            if(EXISTS "${_qt_bin}/${_dep}")
                add_custom_command(TARGET birdy POST_BUILD
                    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                            "${_qt_bin}/${_dep}"
                            "$<TARGET_FILE_DIR:birdy>"
                    VERBATIM
                )
            endif()
        endforeach()
    endif()
endif()

# 鸟检 / 认种与手机端同一套 ncnn 模型。未找到 ncnn 时界面仍可编译，推理接口返回未链接。
find_package(ncnn QUIET)
if(ncnn_FOUND)
    target_compile_definitions(birdy PRIVATE BIRDY_HAS_NCNN=1)
    target_link_libraries(birdy PRIVATE ncnn)
    message(STATUS "BirdyQt: ncnn ${ncnn_VERSION}")
else()
    message(STATUS "BirdyQt: ncnn not found; detection and species id stay unloaded")
endif()
