//========================================================================
//
// pdfimages.cc
//
// Copyright 1998-2003 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "gmem.h"
#include "gmempp.h"
#include "parseargs.h"
#include "GString.h"
#include "GlobalParams.h"
#include "Object.h"
#include "Stream.h"
#include "Array.h"
#include "Dict.h"
#include "XRef.h"
#include "Catalog.h"
#include "Page.h"
#include "PDFDoc.h"
#include "ImageOutputDev.h"
#include "Error.h"
#include "config.h"

static int firstPage;
static int lastPage;
static GBool dumpJPEG;
static GBool dumpRaw;
static GBool list;
static char ownerPassword[33];
static char userPassword[33];
static GBool quiet;
static char cfgFileName[256];
static GBool printVersion;
static GBool printHelp;

static ArgDesc const argDesc[] = {
  {"-f",      argInt,      &firstPage,     0,
   "first page to convert"},
  {"-l",      argInt,      &lastPage,      0,
   "last page to convert"},
  {"-j",      argFlag,     &dumpJPEG,      0,
   "write JPEG images as JPEG files"},
  {"-raw",    argFlag,     &dumpRaw,       0,
   "write raw data in PDF-native formats"},
  {"-list",   argFlag,     &list,          0,
   "write information to stdout for each image"},
  {"-opw",    argString,   ownerPassword,  sizeof(ownerPassword),
   "owner password (for encrypted files)"},
  {"-upw",    argString,   userPassword,   sizeof(userPassword),
   "user password (for encrypted files)"},
  {"-q",      argFlag,     &quiet,         0,
   "don't print any messages or errors"},
  {"-cfg",        argString,      cfgFileName,    sizeof(cfgFileName),
   "configuration file to use in place of .xpdfrc"},
  {"-v",      argFlag,     &printVersion,  0,
   "print copyright and version info"},
  {"-h",      argFlag,     &printHelp,     0,
   "print usage information"},
  {"-help",   argFlag,     &printHelp,     0,
   "print usage information"},
  {"--help",  argFlag,     &printHelp,     0,
   "print usage information"},
  {"-?",      argFlag,     &printHelp,     0,
   "print usage information"},
  {NULL, argFlag, 0, 0, 0}
};

int main(int argc, char *argv[]) {
  PDFDoc *doc;
  char *fileName;
  char *imgRoot;
  GString *ownerPW, *userPW;
  ImageOutputDev *imgOut;
  GBool ok;
  int exitCode;

  exitCode = 99;

  firstPage = 1;
  lastPage = 0;
  dumpJPEG = gFalse;
  dumpRaw = gFalse;
  list = gFalse;
  ownerPassword[0] = '\001';
  userPassword[0] = '\001';
  quiet = gFalse;
  cfgFileName[0] = '\0';
  printVersion = gFalse;
  printHelp = gFalse;

  // parse args
  fixCommandLine(&argc, &argv);
  ok = parseArgs(argDesc, &argc, argv);
  if (!ok || argc != 3 || printVersion || printHelp) {
    fprintf(stderr, "pdfimages version %s\n", xpdfVersion);
    fprintf(stderr, "%s\n", xpdfCopyright);
    if (!printVersion) {
      printUsage("pdfimages", "<PDF-file> <image-root>", argDesc);
    }
    goto err0;
  }
  fileName = argv[1];
  imgRoot = argv[2];

  // read config file
  globalParams = new GlobalParams(cfgFileName);
  if (quiet) {
    globalParams->setErrQuiet(quiet);
  }

  // open PDF file
  if (ownerPassword[0] != '\001') {
    ownerPW = new GString(ownerPassword);
  } else {
    ownerPW = NULL;
  }
  if (userPassword[0] != '\001') {
    userPW = new GString(userPassword);
  } else {
    userPW = NULL;
  }
  doc = new PDFDoc(fileName, ownerPW, userPW);
  if (userPW) {
    delete userPW;
  }
  if (ownerPW) {
    delete ownerPW;
  }
  if (!doc->isOk()) {
    exitCode = 1;
    goto err1;
  }

#if 0
  // check for copy permission
  if (!doc->okToCopy()) {
    error(errNotAllowed, -1,
	  "Copying of images from this document is not allowed.");
    exitCode = 3;
    goto err1;
  }
#endif

  // get page range
  if (firstPage < 1)
    firstPage = 1;
  if (lastPage < 1 || lastPage > doc->getNumPages())
    lastPage = doc->getNumPages();

  // write image files
  imgOut = new ImageOutputDev(imgRoot, dumpJPEG, dumpRaw, list);
  if (imgOut->isOk()) {
    doc->displayPages(imgOut, firstPage, lastPage, 72, 72, 0,
		      gFalse, gTrue, gFalse);
  }
  delete imgOut;

  exitCode = 0;

  // clean up
 err1:
  delete doc;
  delete globalParams;
 err0:

  // check for memory leaks
  Object::memCheck(stderr);
  gMemReport(stderr);

  return exitCode;
}