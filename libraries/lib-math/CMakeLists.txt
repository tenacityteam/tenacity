#[[
A library of mathematical utilities and manipulation of samples
]]#

set( SOURCES
   Dither.cpp
   Dither.h
   FFT.cpp
   FFT.h
   InterpolateAudio.cpp
   InterpolateAudio.h
   Matrix.cpp
   Matrix.h
   RealFFTf.cpp
   RealFFTf.h
   Resample.cpp
   Resample.h
   SampleCount.cpp
   SampleCount.h
   SampleFormat.cpp
   SampleFormat.h
   SSEMathFuncs.cpp
   SSEMathFuncs.h
   Spectrum.cpp
   Spectrum.h
   float_cast.h
)
set( LIBRARIES
   Soxr::soxr
   lib-preferences-interface
   PRIVATE
   wxWidgets::wxWidgets
)
saucedacity_library( lib-math "${SOURCES}" "${LIBRARIES}"
   "" ""
)
