# Avoid multiple passes and generation of makefiles
CONFIG -= debug_and_release

defineTest(cloneAndClean) {
    DIRECTORY = $${1}
    REPOSITORY = $${2}
    CHECKOUT = $${3}

    !exists($${DIRECTORY}) {
        system(git clone $${REPOSITORY} $${DIRECTORY})
        !isEmpty(CHECKOUT) {
            MESSAGE = Checking out $${CHECKOUT}
            COMMAND = cd $${DIRECTORY} && git checkout $${CHECKOUT}
            contains(CHECKOUT, ".*tags/.*") {
                BRANCH = $$replace(CHECKOUT, "tags/", "Branch_")
            }
            !isEmpty(BRANCH) {
                MESSAGE += into $${BRANCH}
                COMMAND += -b $$BRANCH
            }
            message($${MESSAGE})
            system($${COMMAND})
        }
    }

    system(cd $${DIRECTORY} && git clean -dffx)
}

defineTest(buildDebugAndRelease) {
    DIRECTORY = $${1}
    CMAKEPARAMS = $${2}

    win32 {
        JOMPATH = $$system(where jom)
        isEmpty(JOMPATH) {
            MAKECMD = nmake
            CMAKEGEN = $$shell_quote(NMake Makefiles)
        } else {
            MAKECMD = jom
            CMAKEGEN = $$shell_quote(NMake Makefiles JOM)
        }
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
