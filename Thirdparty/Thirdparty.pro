# Avoid multiple passes and generation of makefiles
CONFIG -= debug_and_release

win32 {
    MAKECMD = nmake
    CMAKEGEN = $$shell_quote(NMake Makefiles)
}
unix {
    MAKECMD = make
    CMAKEGEN = $$shell_quote(Unix Makefiles)
}

message(MAKECMD=$${MAKECMD})
message(CMAKEGEN=$${CMAKEGEN})

# =====  OpenGL Mathematics (GLM) = http://glm.g-truc.net =========================================

!exists(glm) {
    system(git clone https://github.com/g-truc/glm.git glm)
    system(cd glm && git checkout 0.9.6)
}

# =====  Open Asset Import Library = http://assimp.sf.net =========================================

!exists(assimp) {
    system(git clone https://github.com/assimp/assimp.git assimp)
}

CMAKEFULL = cd assimp && cmake -G $${CMAKEGEN}
CMAKEFULL += -DBUILD_SHARED_LIBS=OFF
CMAKEFULL += -DASSIMP_BUILD_ASSIMP_TOOLS=OFF
CMAKEFULL += -DBUILD_TESTS=OFF
message(CMAKEFULL(assimp)=$${CMAKEFULL})

system($${CMAKEFULL} -DCMAKE_BUILD_TYPE=Debug)
system(cd assimp && $${MAKECMD})

system($${CMAKEFULL} -DCMAKE_BUILD_TYPE=Release)
system(cd assimp && $${MAKECMD})

# =====  Bullet Physics Library = http://bulletphysics.org  =======================================

!exists(bullet) {
    system(git clone https://github.com/bulletphysics/bullet3.git bullet)
}

CMAKEFULL = cd bullet && cmake -G $${CMAKEGEN}
CMAKEFULL += -DUSE_MSVC_RUNTIME_LIBRARY_DLL=ON
CMAKEFULL += -DBUILD_SHARED_LIBS=OFF
CMAKEFULL += -DUSE_MSVC_FAST_FLOATINGPOINT=OFF
CMAKEFULL += -DBUILD_BULLET2_DEMOS=OFF
CMAKEFULL += -DBUILD_BULLET3=OFF
CMAKEFULL += -DBUILD_CPU_DEMOS=OFF
CMAKEFULL += -DBUILD_EXTRAS=OFF
CMAKEFULL += -DBUILD_OPENGL3_DEMOS=OFF
CMAKEFULL += -DBUILD_UNIT_TESTS=OFF
message(CMAKEFULL(bullet)=$${CMAKEFULL})

system($${CMAKEFULL} -DCMAKE_BUILD_TYPE=Debug)
system(cd bullet && $${MAKECMD})

system($${CMAKEFULL} -DCMAKE_BUILD_TYPE=Release)
system(cd bullet && $${MAKECMD})
