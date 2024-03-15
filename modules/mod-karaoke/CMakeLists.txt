set( SOURCES
    Lyrics.cpp
    Lyrics.h
    LyricsMain.cpp
    LyricsMain.h
    LyricsWindow.cpp
    LyricsWindow.h
)

tenacity_module(mod-karaoke "${SOURCES}" "Tenacity" "" "")
