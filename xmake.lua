add_rules("mode.debug", "mode.release")
package("ncnn_shared")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/Tencent/ncnn")
    set_description("ncnn is a high-performance neural network inference framework optimized for the mobile platform")
    set_license("BSD-3-Clause")

    if is_plat("windows") then 
        add_versions("20240102", "75a25ef5878376e5a11193f1fb196b0e7ad1dcfa5fe235a78bba581c9a94f999")
        add_urls("https://github.com/Tencent/ncnn/releases/download/20240102/ncnn-20240102-windows-vs2022-shared.zip")
    end
    add_includedirs("include")
    on_install("windows|x64", function (package)
        os.cp("x64/include/*", package:installdir("include"))
        os.cp("x64/lib/*", package:installdir("lib"))
        os.cp("x64/bin/*", package:installdir("bin"))
    end)
    on_install("windows|x32", function (package)
        os.cp("x32/include/*", package:installdir("include"))
        os.cp("x32/lib/*", package:installdir("lib"))
        os.cp("x32/bin/*", package:installdir("bin"))
    end)
    on_install("windows|arm64", function (package)
        os.cp("arm64/include/*", package:installdir("include"))
        os.cp("arm64/lib/*", package:installdir("lib"))
        os.cp("arm64/bin/*", package:installdir("bin"))
    end)
    on_install("windows|arm", function (package)
        os.cp("arm/include/*", package:installdir("include"))
        os.cp("arm/lib/*", package:installdir("lib"))
        os.cp("arm/bin/*", package:installdir("bin"))
    end)
    on_load("mingw", "windows", function (package)
        if package:config("shared") then
            package:addenv("PATH", "bin")
        end
    end)
package_end()

package("openslide")
    set_kind("library")
    set_homepage("https://openslide.org/")
    set_description("OpenSlide is a C library that provides a simple interface to read whole-slide images (also known as virtual slides).")
    set_license("GNU Lesser General Public License, version 2.1")
    if is_plat("windows") then 
        if is_arch("x64") then
            add_versions("20231011","c32ec4244f4023b5721b89df1d6bd9d2ce670db45fdfadf5df3582db9d5cbeb7")
            add_urls("https://github.com/openslide/openslide-bin/releases/download/v$(version)/openslide-win64-$(version).zip")
        elseif is_arch("x86") then
            add_versions("20231011","8b7f374647bab0cd92fe7378d6a7197b684da3e2ab3de3efd0995987e003e4f6")
            add_urls("https://github.com/openslide/openslide-bin/releases/download/v$(version)/openslide-win32-$(version).zip")
        end 
    end 
    on_install("windows", function (package)
        os.cp("bin/*", package:installdir("bin"))
        os.cp("include/*", package:installdir("include"))
        os.cp("lib/*", package:installdir("lib"))
    end)

    on_load("windows", function (package)
        package:addenv("PATH", "bin")
    end)
package_end()

set_runtimes("MT")
add_requires("ncnn_shared", {configs = {shared = true}})
add_requires("opencv","libtiff", "minizip", "openslide", "libzip")
add_cxflags("/EHsc")
-- add_requires("itk")
CONFIG_VERSION = "beta 0.2"
set_languages("c11", "c++17")
set_optimize("fastest")
console = false
target("CellScope")
    if(console) then
        add_ldflags("/SUBSYSTEM:CONSOLE") 
    end
    add_rules("qt.widgetapp")
    add_headerfiles("src/*.h","src/io/*.h", "src/scopeFile/*.h","src/ui/*.h", "src/onRequestTask/*.h")

    add_packages("ncnn_shared")
    add_packages("opencv","libtiff","minizip","openslide", "libzip")

    add_deps("AIBackend")

    add_files("res/*.qrc")
    add_files("src/ui/mainwindow.ui", "src/ui/AIProcessWindow.ui")
    
    add_files("src/*.cpp","src/io/*.cpp", "src/scopeFile/*.cpp", "src/ui/*.cpp", "src/onRequestTask/*.cpp")

    add_files("src/ui/*.h", "src/onRequestTask/*.h", "src/*.h")
    add_includedirs("$(buildir)/config")
    set_configdir("$(buildir)/config")
    add_configfiles("src/Config.h.in", {variables = {CONFIG_VERSION = CONFIG_VERSION, CONFIG_NAME = "CellScope"}})
    -- add files with Q_OBJECT meta (only for qt.moc)

target("AIBackend")
    add_rules("qt.shared")
    if(console) then
        add_ldflags("/SUBSYSTEM:CONSOLE") 
    end


    add_packages("ncnn_shared", "opencv")


    add_headerfiles("src/AIBackend/*.h", "src/AIBackend/zgnUNetForVIM/*.h")

    add_includedirs("src/AIBackend","scr/AIBackend/zgnUNetForVIM", {interface = true})
    add_files("src/AIBackend/*.cpp","src/AIBackend/zgnUNetForVIM/*.cpp")

    if is_plat("windows") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end
