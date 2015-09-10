win32 {
    MAKECMD = nmake
    CMAKEGEN = $$shell_quote('NMake Makefiles')
}
unix {
    MAKECMD = make
    CMAKEGEN = $$shell_quote('Unix Makefiles')
}

# =====  OpenGL Mathematics (GLM) = http://glm.g-truc.net =========================================

!exists("glm") {
    system("git clone https://github.com/g-truc/glm.git glm")
    system("cd glm && git checkout 0.9.6")
}

# =====  Open Asset Import Library = http://assimp.sf.net =========================================

!exists("assimp") {
    system("git clone https://github.com/assimp/assimp.git assimp")
    CMAKEFULL = "cd assimp && cmake -G $${CMAKEGEN} -DBUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_ASSIMP_TOOLS=OFF"

    system($$CMAKEFULL -DCMAKE_BUILD_TYPE=Debug)
    system("cd assimp && $${MAKECMD}")

    system($$CMAKEFULL -DCMAKE_BUILD_TYPE=Release)
    system("cd assimp && $${MAKECMD}")
}

# =====  Bullet Physics Library = http://bulletphysics.org  =======================================

!exists("bullet") {
    system("git clone https://github.com/bulletphysics/bullet3.git bullet")
    CMAKEFULL = "cd bullet && cmake -G $${CMAKEGEN} -DBUILD_SHARED_LIBS=OFF -DUSE_MSVC_FAST_FLOATINGPOINT=OFF"

    system($$CMAKEFULL -DCMAKE_BUILD_TYPE=Debug)
    system("cd bullet && $${MAKECMD}")

    system($$CMAKEFULL -DCMAKE_BUILD_TYPE=Release)
    system("cd bullet && $${MAKECMD}")
}