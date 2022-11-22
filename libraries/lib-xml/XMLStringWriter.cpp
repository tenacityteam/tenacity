/**********************************************************************

  Tenacity: A Digital Audio Editor

  XMLStringWriter.cpp

  Avery King split from XMLWriter.cpp

**********************************************************************/
#include "XMLStringWriter.h"

XMLStringWriter::XMLStringWriter(size_t initialSize)
{
   if (initialSize)
   {
      reserve(initialSize);
   }
}

XMLStringWriter::~XMLStringWriter()
{
}

void XMLStringWriter::Write(const wxString &data)
{
   Append(data);
}
