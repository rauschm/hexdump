/****************************************************************************/
/*  H E X D U M P . C                                                       */
/*                                                                          */
/*  Das Programm erzeugt einen Hexdump einer Datei. Dabei wird entweder der */
/*  Dateiname in der Kommandozeile übergeben oder der Dateiinhalt über die  */
/*  umgeleitete Standardeingabe eingelesen.                                 */
/*                                                                          */
/*  Aufruf: hexdump [<] Datei                                               */
/****************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#define _setmode(x,y)
#else
#include <fcntl.h>
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#endif


#include "hexdump_proto.h"


int main(int argc, char* argv[])
{
  if (argc == 2)
    dumpNamedFile(argv[1]);
  else
    if (argc < 2 && !isatty(fileno(stdin)))
      dumpFile(useStdin(), "<stdin>");
    else
      usage();
  return 0;
}


void dumpNamedFile(char* fileName)
{
  FILE* f = openFile(fileName);
  dumpFile(f, fileName);
  closeFile(f);
}


void dumpFile(FILE* f, char* fileName)
{
  unsigned char fileBuffer[16 * 0x1000];
  int           fileOffset;
  int           bufferLength;
  int           error;

  fileOffset = 0;
  do
  {
    bufferLength = fread(fileBuffer, 1, sizeof(fileBuffer), f);
    dumpBuffer(fileOffset, fileBuffer, bufferLength);
    fileOffset += bufferLength;
  }
  while (bufferLength == sizeof(fileBuffer));
  checkFileError(f, fileName);
}


void dumpBuffer(int fileOffset, unsigned char* buffer, int bufferLength)
{
  for (int i = 0; i < bufferLength; i += 16, fileOffset += 16)
    dumpLine(fileOffset, &buffer[i], bufferLength - i);
}


void dumpLine(int fileOffset, unsigned char* buffer, int availableCharsInBuffer)
{
  char hexArea[16 * 3 + 1];
  char asciiArea[16 * 1 + 1];

  if (availableCharsInBuffer > 16)
    availableCharsInBuffer = 16;
  for (int i = 0; i < availableCharsInBuffer; i++)
  {
    dumpCharAsHex(&hexArea[3 * i], buffer[i]);
    dumpCharAsAscii(&asciiArea[i], buffer[i]);
  }
  hexArea[3 * availableCharsInBuffer] = '\0';
  asciiArea[availableCharsInBuffer] = '\0';
  printf("%08x:%-48s  %s\n", fileOffset, hexArea, asciiArea);
}


void dumpCharAsHex(char* hexArea, unsigned char c)
{
  hexArea[0] = ' ';
  hexArea[1] = c / 16 + ((c / 16 < 10) ? '0' : 'A' - 10);
  hexArea[2] = c % 16 + ((c % 16 < 10) ? '0' : 'A' - 10);
}


void dumpCharAsAscii(char* asciiArea, unsigned char c)
{
  asciiArea[0] = (c < 32 || 127 <= c && c < 160) ? '.' : c;
}


void usage(void)
{
  exitWithErrorMessage("Gibt den Inhalt der Datei im Hex-/Ascii-Format aus\n"
                       "Aufruf: hexdump [<] Datei\n");
}


FILE* openFile(char* fileName)
{
  FILE* f;

  if ((f = fopen(fileName, "rb")) == NULL)
    exitWithErrorMessage("hexdump: open error %s: %s", fileName, strerror(errno));
  return f;
}


FILE* useStdin(void)
{
  _setmode(fileno(stdin), _O_BINARY);
  return stdin;
}


void closeFile(FILE* f)
{
  if (f != stdin)
    fclose(f);
}


void checkFileError(FILE* f, char* fileName)
{
  if (ferror(f))
  {
    int err = errno;
    closeFile(f);
    exitWithErrorMessage("hexdump: read error %s: %s\n", fileName, strerror(err));
  }
}


void exitWithErrorMessage(char* format, ...)
{
  va_list args;

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  exit(1);
}
