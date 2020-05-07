
workspace "CGPUCRS"
    architecture "x64"
    configurations { "Debug", "Release" }
    includedirs "./SOIL"
    includedirs "./Temporizador"
    includedirs "./ImageClass"
    links {
        "GL", "GLU", "glut", "pthread"
    }
    buildoptions {"-std=c++17" }
    filter { "configurations:Debug" }
        defines { "DEBUG" }
        buildoptions { "-Wall" }
        symbols "On"
    
    filter { "configurations:Release" }
        optimize "On"
    
project "ExemploDeManipulacaoDeImagens"
    links { "ImageClass",  "SOIL", "Temporizador" }
    kind "ConsoleApp"
    files { "ExemploDeManipulacaoDeImagens.cpp" }
   
project "Histogram"
    links { "ImageClass", "SOIL", "Temporizador" }
    kind "ConsoleApp"
    files { "Histogram.cpp" }

   
project "Windowing"
    links { "ImageClass", "SOIL", "Temporizador" }
    kind "ConsoleApp"
    files { "Windowing.cpp" }

project "SegmentationByThreshold"
    links { "ImageClass", "SOIL", "Temporizador" }
    kind "ConsoleApp"
    files { "SegmentationByThreshold.cpp" }

project "TextureSegmentation"
    links { "ImageClass", "SOIL", "Temporizador" }
    kind "ConsoleApp"
    files { "TextureSegmentation.cpp" }

project "SOIL"
    kind "StaticLib"
    files { "./SOIL/*.cpp" }

project "ImageClass"
    links { "SOIL" }
    kind "StaticLib"
    files { "./ImageClass/*.cpp" }    

project "Temporizador"
    kind "StaticLib"
    files { "./Temporizador/*.cpp" }