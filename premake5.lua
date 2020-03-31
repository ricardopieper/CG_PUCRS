
workspace "CGPUCRS"
    architecture "x64"
    configurations { "Debug", "Release" }



    
project "ExemploDeManipulacaoDeImagens"
    includedirs "./SOIL"
    includedirs "."

    kind "ConsoleApp"
    
    configurations { "Debug", "Release" }
    links {
        "GL", "GLU", "glut"
    }
    
    files { "**.cpp" }
    
    buildoptions {"-std=c++17" }
    
    filter { "configurations:Debug" }
        defines { "DEBUG" }
        buildoptions { "-Wall" }
        symbols "On"
    
    filter { "configurations:Release" }
        defines { "NDEBUG" }
        optimize "On"