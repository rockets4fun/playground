include(Common.inc)

# ===== Open Asset Import Library = http://assimp.sf.net ===========================================

cloneAndClean(Sdl, https://github.com/libsdl-org/SDL.git, release-2.0.20)

PARAMS  = -B. -H..
PARAMS += -DCMAKE_DEBUG_POSTFIX=_Debug
PARAMS += -DSDL_SHARED=OFF
PARAMS += -DDIRECTX=OFF

system(cd Sdl && mkdir Build)
buildDebugAndRelease(Sdl/Build, $${PARAMS})
