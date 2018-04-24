include(Common.inc)

# ===== Bullet Physics Library = http://bulletphysics.org  =========================================

cloneAndClean(Bullet, https://github.com/bulletphysics/bullet3.git, tags/2.87)

PARAMS  = -DUSE_MSVC_RUNTIME_LIBRARY_DLL=ON
PARAMS += -DBUILD_SHARED_LIBS=OFF
PARAMS += -DUSE_MSVC_FAST_FLOATINGPOINT=OFF
PARAMS += -DBUILD_BULLET2_DEMOS=OFF
PARAMS += -DBUILD_BULLET3=OFF
PARAMS += -DBUILD_CPU_DEMOS=OFF
PARAMS += -DBUILD_EXTRAS=OFF
PARAMS += -DBUILD_OPENGL3_DEMOS=OFF
PARAMS += -DBUILD_UNIT_TESTS=OFF

buildDebugAndRelease(Bullet, $${PARAMS})
