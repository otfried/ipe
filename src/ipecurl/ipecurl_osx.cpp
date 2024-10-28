// -*- objc -*-
// --------------------------------------------------------------------
// ipecurl for OSX
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2024 Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "ipebase.h"

#import <Foundation/Foundation.h>

using namespace ipe;

#define TEXNAME "ipetemp.tex"
#define PDFNAME "ipetemp.pdf"
#define LOGNAME "ipetemp.log"
#define URLNAME "url1.txt"

// --------------------------------------------------------------------

inline NSString * I2N(String s) { return [NSString stringWithUTF8String:s.z()]; }

static void usage() {
    fprintf(
	stderr,
	"Usage: ipecurl [ pdflatex | xelatex | lualatex ]\n"
	"Ipecurl runs a Latex compilation on an cloud service given in 'url1.txt'.\n");
    exit(1);
}

// --------------------------------------------------------------------

int main(int argc, char * argv[]) {
    ipe::Platform::initLib(ipe::IPELIB_VERSION);

    // ensure one arguments
    if (argc != 2) usage();

    String command = argv[1];

    if (command != "pdflatex" && command != "xelatex" && command != "lualatex") {
	fprintf(stderr, "Illegal Latex command.\n");
	return -1;
    }

    String url = Platform::readFile(URLNAME);
    if (url.empty() || url.left(4) != "http") {
	fprintf(stderr, "Cannot find url for cloud service in '%s'.\n", URLNAME);
	return -2;
    }

    String tex = Platform::readFile(TEXNAME);

    if (tex.empty()) {
	fprintf(stderr, "Cannot read Latex source from '%s'.\n", TEXNAME);
	return -3;
    }

    while (url.right(1) == "\n" || url.right(1) == " " || url.right(1) == "\r")
	url = url.left(url.size() - 1);

    url += "/data?target=ipetemp.tex&command=";
    url += command;

    // ipeDebug("URL: '%s'", url.z());

    String boundary{"------------------------f0324ce8daa3cc53"};
    String mime;
    StringStream ss(mime);
    ss << "--" << boundary << "\r\n"
       << "Content-Disposition: form-data; name=\"file\"; "
	  "filename=\"latexTarball.tar\"\r\n"
       << "Content-Type: text/plain\r\n\r\n";
    // need to send Latex source as a tarball
    ss << Platform::createTarball(tex) << "\r\n";
    ss << "--" << boundary << "--\r\n";

    // It is essential to set the user agent, otherwise latexonline will
    // believe we are a web browser, and return some Javascript code that
    // says "Compiling Latex" and redirects to the PDF later...

    auto agent = [NSString stringWithFormat:@"ipecurl_osx %d", Platform::libVersion()];

    auto configuration = [NSURLSessionConfiguration defaultSessionConfiguration];
    configuration.HTTPAdditionalHeaders = @{@"User-Agent" : agent};

    auto session = [NSURLSession sessionWithConfiguration:configuration
						 delegate:nil
					    delegateQueue:nil];

    String contentType{"multipart/form-data; boundary="};
    contentType += boundary;

    NSURL * nsUrl = [NSURL URLWithString:I2N(url)];
    NSMutableURLRequest * request = [NSMutableURLRequest requestWithURL:nsUrl];
    request.HTTPMethod = @"POST";
    [request setValue:I2N(contentType) forHTTPHeaderField:@"Content-Type"];
    request.HTTPBody = [NSData dataWithBytes:mime.data() length:mime.size()];

    __block String pdf;

    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    auto task = [session
	dataTaskWithRequest:request
	  completionHandler:^(NSData * data, NSURLResponse * response, NSError * error) {
	    if (data) {
		pdf = String((const char *)data.bytes, data.length);
	    } else {
		StringStream err(pdf);
		err << "! A network error occurred using the Latex cloud service\n";
		err << "Code:   " << int(error.code) << "\n";
		err << "Domain: " << error.domain.UTF8String << "\n";
		err << "Error:  " << error.localizedDescription.UTF8String << "\n";
	    }
	    dispatch_semaphore_signal(semaphore);
	  }];

    [task resume];
    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);

    // generate logfile
    FILE * log = Platform::fopen(LOGNAME, "wb");
    fprintf(log, "entering extended mode: using latexonline at '%s'\n", url.z());

    if (pdf.left(4) == "%PDF") {
	FILE * out = fopen(PDFNAME, "wb");
	if (!out) {
	    fprintf(stderr, "Cannot open '%s' for writing.\n", PDFNAME);
	    return -4;
	}
	fwrite(pdf.data(), 1, pdf.size(), out);
	fclose(out);
    } else {
	// an error happened during Latex run: pdf is actually a log
	fwrite(pdf.data(), 1, pdf.size(), log);
    }
    fclose(log);
    return 0;
}

// --------------------------------------------------------------------
