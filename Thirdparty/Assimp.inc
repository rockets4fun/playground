include(Common.inc)

# =====  Open Asset Import Library = http://assimp.sf.net =========================================

cloneAndClean(Assimp, https://github.com/assimp/assimp.git)

PARAMS  = -DBUILD_SHARED_LIBS=OFF
PARAMS += -DASSIMP_BUILD_ASSIMP_TOOLS=OFF
PARAMS += -DBUILD_TESTS=OFF

buildDebugAndRelease(Assimp, $${PARAMS})
