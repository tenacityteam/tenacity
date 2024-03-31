
set( TARGET minsrc )
set( TARGET_ROOT ${CMAKE_SOURCE_DIR} )

message( STATUS "========== Configuring ${TARGET} ==========" )

def_vars()

# Add regular expressions of files/directories to exclude from the tarball
#
# NOTE: Each item will be wrapped with "^" and ".*$"
list( APPEND EXCLUDES
   .git
   .gitattributes
   .github
   .gitea
   .gitignore
   .woodpecker
   mac
   plug-ins/analyze.ny
   plug-ins/fadein.ny
   plug-ins/fadeout.ny
   plug-ins/undcbias.ny
   qa
   tests/ProjectCheckTests
   tests/samples
   todo.txt
   win
)

set( TLD "tenacity-minsrc-${GIT_DESCRIBE}" )
set( TARBALL "${PROJECT_BINARY_DIR}/${TLD}.tar.xz" )
   
add_custom_target( ${TARGET}
   COMMAND
      ${CMAKE_COMMAND} -D GIT_EXECUTABLE="${GIT_EXECUTABLE}"
                       -D TARGET_ROOT="${TARGET_ROOT}"
                       -D EXCLUDES="${EXCLUDES}"
                       -D TARBALL="${TARBALL}"
                       -D TLD="${TLD}"
                       -P "${_SRCDIR}/maketarball.cmake"
)

