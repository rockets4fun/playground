# Avoid multiple passes and generation of makefiles
CONFIG -= debug_and_release

defineTest(cloneAndClean) {
    DIRECTORY = $${1}
    REPOSITORY = $${2}

    !exists($${DIRECTORY}) {
        system(git clone $${REPOSITORY} $${DIRECTORY})
    }

    system(cd $${DIRECTORY} && git clean -dffx)
}

defineTest(buildDebugAndRelease) {
    DIRECTORY = $${1}
    CMAKEPARAMS = $${2}

    win32 {
        MAKECMD = nmake
        CMAKEGEN = $$shell_quote(NMake Makefiles)
    }
    unix {
        MAKECMD = make
        CMAKEGEN = $$shell_quote(Unix Makefiles)
    }

    CMAKEFULL = cmake -G $${CMAKEGEN} $${CMAKEPARAMS}

    system(cd $${DIRECTORY} && $${CMAKEFULL} -DCMAKE_BUILD_TYPE=Debug)
    system(cd $${DIRECTORY} && $${MAKECMD})

    system(cd $${DIRECTORY} && $${CMAKEFULL} -DCMAKE_BUILD_TYPE=Release)
    system(cd $${DIRECTORY} && $${MAKECMD})
}
