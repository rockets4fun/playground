include(Common.inc)

# ===== Open Asset Import Library = http://assimp.sf.net ===========================================

cloneAndClean(Sdl, https://github.com/spurious/SDL-mirror.git, release-2.0.8)

PARAMS  = -B. -H..
PARAMS += -DCMAKE_DEBUG_POSTFIX=_Debug
PARAMS += -DSDL_SHARED=OFF
PARAMS += -DDIRECTX=OFF

system(cd Sdl && mkdir Build)
buildDebugAndRelease(Sdl/Build, $${PARAMS})
