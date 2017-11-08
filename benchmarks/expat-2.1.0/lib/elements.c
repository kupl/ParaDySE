/* This is simple demonstration of how to use expat. This program
   reads an XML document from standard input and writes a line with
   the name of each element to standard output indenting child
   elements by one tab stop more than their parent element.
   It must be used with Expat compiled for UTF-8 output.
*/

#include <crest.h>
#include <stdio.h>
#include <string.h>
#include "expat.h"

#if defined(__amigaos__) && defined(__USE_INLINE__)
#include <proto/expat.h>
#endif

#ifdef XML_LARGE_SIZE
#if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
#define XML_FMT_INT_MOD "I64"
#else
#define XML_FMT_INT_MOD "ll"
#endif
#else
#define XML_FMT_INT_MOD "l"
#endif

int SYMINPUT_SIZE;

void *
sym_memcpy(void *dst, const void *src, size_t len)
{
        size_t i;
        char *d = dst;
        const char *s = src;

        for (i=0; i<len; i++) {
                d[i] = s[i];
        }

        return dst;
}

int
memcmp(const void *Ptr1, const void *Ptr2, size_t Count)
{
    int v = 0;
    char *p1 = (char *)Ptr1;
    char *p2 = (char *)Ptr2;

    while(Count-- > 0 && v == 0) {
        v = *(p1++) - *(p2++);
    }

    return v;
}


static void XMLCALL
startElement(void *userData, const char *name, const char **atts)
{
  int i;
  int *depthPtr = (int *)userData;
  for (i = 0; i < *depthPtr; i++)
    putchar('\t');
  puts(name);
  *depthPtr += 1;
}

static void XMLCALL
endElement(void *userData, const char *name)
{
  int *depthPtr = (int *)userData;
  *depthPtr -= 1;
}

static void XMLCALL
myDataHandler(void *userData, const XML_Char *s, int len)
{
  return;
}

static void XMLCALL
myCommentHandler(void *userData, const XML_Char *s)
{
  return;
}

static void XMLCALL
myDefaultHandler(void *userdata, const XML_Char *s, int len)
{
  return;
}

int XMLCALL
myUnknownEncodingHandler(void *a, const XML_Char *b, XML_Encoding *info)
{
  return 0;
}

static void XMLCALL
myVoidHandler(void *userdata)
{
  return;
}

static void XMLCALL
myNamespaceDeclHandler(void *userData,
                                 const XML_Char *prefix,
                                 const XML_Char *uri) {
  return;
}

static void XMLCALL
myUnparsedEntityDeclHandler(void *userData,
                                 const XML_Char *entityName,
                                 const XML_Char *base,
                                 const XML_Char *systemId,
                                 const XML_Char *publicId,
                                 const XML_Char *notationName) {
  return;
}

static void XMLCALL
myProcessingInstruction(void *userData,
                                    const XML_Char *target,
                                    const XML_Char *data) {
  return;
}

int
main(int argc, char *argv[])
{
  char *buf;
  char a = 'A';
  XML_Parser parser = XML_ParserCreateNS("UTF-8", ":");
  int done;
  int depth = 0;
  XML_SetUserData(parser, &depth);
  XML_SetElementHandler(parser, startElement, endElement);
  XML_SetCharacterDataHandler(parser, myDataHandler);
  XML_SetCommentHandler(parser, myCommentHandler);
  XML_SetDefaultHandler(parser, myDefaultHandler);
  XML_SetUnknownEncodingHandler(parser, myUnknownEncodingHandler, (void*)&a);
  XML_SetCdataSectionHandler(parser, myVoidHandler, myVoidHandler);
  XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_GetSpecifiedAttributeCount(parser);
  XML_SetNamespaceDeclHandler(parser, myNamespaceDeclHandler, myNamespaceDeclHandler);
  XML_SetUnparsedEntityDeclHandler(parser, myUnparsedEntityDeclHandler);
  XML_SetProcessingInstructionHandler(parser, myProcessingInstruction);

	SYMINPUT_SIZE = atoi(argv[2]);
	buf = (char*)malloc(SYMINPUT_SIZE);

	if (XML_Parse(parser, buf, SYMINPUT_SIZE, 1) == XML_STATUS_ERROR) {
		fprintf(stderr, "PARSE ERROR:%s\n", buf);
		/*
		fprintf(stderr,
						"%s at line %" XML_FMT_INT_MOD "u\n",
						XML_ErrorString(XML_GetErrorCode(parser)),
						XML_GetCurrentLineNumber(parser));
		*/
		return 1;
	}
  XML_ParserFree(parser);
  return 0;
}
