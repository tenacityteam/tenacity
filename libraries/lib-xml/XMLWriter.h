/**********************************************************************

  Audacity: A Digital Audio Editor

  XMLWriter.h

  Leland Lucius, Dmitry Vedenko

**********************************************************************/
#ifndef __AUDACITY_XML_XML_FILE_WRITER__
#define __AUDACITY_XML_XML_FILE_WRITER__

#include <vector>
#include <wx/ffile.h> // to inherit

#include <string_view> // For UTF8 writer
#include "MemoryStream.h"

#include "FileException.h"

#include "Identifier.h"

///
/// XMLWriter
///
class XML_API XMLWriter /* not final */ {

 public:

   XMLWriter();
   virtual ~XMLWriter();

   virtual void StartTag(const wxString &name);
   virtual void EndTag(const wxString &name);

   // nonvirtual pass-through
   void WriteAttr(const wxString &name, const Identifier &value)
      // using GET once here, permitting Identifiers in XML,
      // so no need for it at each WriteAttr call
      { WriteAttr( name, value.GET() ); }

   virtual void WriteAttr(const wxString &name, const wxString &value);
   virtual void WriteAttr(const wxString &name, const wxChar *value);

   virtual void WriteAttr(const wxString &name, int value);
   virtual void WriteAttr(const wxString &name, bool value);
   virtual void WriteAttr(const wxString &name, long value);
   virtual void WriteAttr(const wxString &name, long long value);
   virtual void WriteAttr(const wxString &name, size_t value);
   virtual void WriteAttr(const wxString &name, float value, int digits = -1);
   virtual void WriteAttr(const wxString &name, double value, int digits = -1);

   virtual void WriteData(const wxString &value);

   virtual void WriteSubTree(const wxString &value);

   virtual void Write(const wxString &data) = 0;

   // Escape a string, replacing certain characters with their
   // XML encoding, i.e. '<' becomes '&lt;'
   wxString XMLEsc(const wxString & s);

 protected:

   bool mInTag;
   int mDepth;
   wxArrayString mTagstack;
   std::vector<int> mHasKids;

};

class XML_API XMLUtf8BufferWriter final
{
public:
   void StartTag(const std::string_view& name);
   void EndTag(const std::string_view& name);

   void WriteAttr(const std::string_view& name, const Identifier& value);
   // using GET once here, permitting Identifiers in XML,
   // so no need for it at each WriteAttr call

   void WriteAttr(const std::string_view& name, const std::string_view& value);

   void WriteAttr(const std::string_view& name, int value);
   void WriteAttr(const std::string_view& name, bool value);
   void WriteAttr(const std::string_view& name, long value);
   void WriteAttr(const std::string_view& name, long long value);
   void WriteAttr(const std::string_view& name, size_t value);
   void WriteAttr(const std::string_view& name, float value, int digits = -1);
   void WriteAttr(const std::string_view& name, double value, int digits = -1);

   void WriteData(const std::string_view& value);

   void WriteSubTree(const std::string_view& value);

   void Write(const std::string_view& value);

   MemoryStream ConsumeResult();
private:
   void WriteEscaped(const std::string_view& value);

   MemoryStream mStream;

   bool mInTag { false };
};

#endif
