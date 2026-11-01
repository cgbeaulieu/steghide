/*
 * steghide 0.5.1 - a steganography program
 * Copyright (C) 1999-2003 Stefan Hetzl <shetzl@chello.at>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "WavPCMSampleValueTest.h"
#include "utcommon.h"

#include "WavPCMSampleValue.h"
#include "CvrStgFile.h"

WavPCMSampleValueTest::WavPCMSampleValueTest (TestSuite* s)
	: SampleValueTest ("WavPCMSampleValue", s)
{
	ADDTESTCATEGORY (WavPCMSampleValueTest, testDistance) ;
	ADDTESTCATEGORY (WavPCMSampleValueTest, testIsNeighbour) ;
	ADDTESTCATEGORY (WavPCMSampleValueTest, testNearestTarget) ;
}

void WavPCMSampleValueTest::setup ()
{
	UnitTest::setup() ;

	Globs.reset() ;
	f_WavPCM8 = CvrStgFile::readFile (std::string(DATADIR) + "pcm8_std.wav").release() ;
	sv_WavPCM8_0 = new WavPCMSampleValue (0) ;
	sv_WavPCM8_1 = new WavPCMSampleValue (1) ;
	sv_WavPCM8_45 = new WavPCMSampleValue (45) ;
	sv_WavPCM8_255 = new WavPCMSampleValue (255) ;
	gl_WavPCM8 = Globs ;

	Globs.reset() ;
	f_WavPCM16 = CvrStgFile::readFile (std::string(DATADIR) + "pcm16_std.wav").release() ;
	sv_WavPCM16_m32768 = new WavPCMSampleValue (-32768) ;
	sv_WavPCM16_32767 = new WavPCMSampleValue (32767) ;
	sv_WavPCM16_0 = new WavPCMSampleValue (0) ;
	sv_WavPCM16_15 = new WavPCMSampleValue (15) ;
	gl_WavPCM16 = Globs ;
}

void WavPCMSampleValueTest::cleanup ()
{
	UnitTest::cleanup() ;

	delete f_WavPCM8 ; delete f_WavPCM16 ;
	delete sv_WavPCM8_0 ; delete sv_WavPCM8_1 ; delete sv_WavPCM8_45 ; delete sv_WavPCM8_255 ;
	delete sv_WavPCM16_m32768 ; delete sv_WavPCM16_32767 ; delete sv_WavPCM16_0 ; delete sv_WavPCM16_15 ;
}

void WavPCMSampleValueTest::testDistance ()
{
	Globs = gl_WavPCM8 ;
	addTestResult (genericTestDistance (sv_WavPCM8_0, sv_WavPCM8_1, 1)) ;
	addTestResult (genericTestDistance (sv_WavPCM8_1, sv_WavPCM8_45, 44)) ;

	Globs = gl_WavPCM16 ;
	addTestResult (genericTestDistance (sv_WavPCM16_0, sv_WavPCM16_15, 15)) ;
	addTestResult (genericTestDistance (sv_WavPCM16_15, sv_WavPCM16_32767, 32752)) ;
	addTestResult (genericTestDistance (sv_WavPCM16_m32768, sv_WavPCM16_32767, 65535)) ;
}

void WavPCMSampleValueTest::testIsNeighbour ()
{
	Globs = gl_WavPCM8 ;
	addTestResult (genericTestIsNeighbour (sv_WavPCM8_0, sv_WavPCM8_1, true)) ;
	addTestResult (genericTestIsNeighbour (sv_WavPCM8_0, sv_WavPCM8_45, false)) ;

	Globs = gl_WavPCM16 ;
	addTestResult (genericTestIsNeighbour (sv_WavPCM16_0, sv_WavPCM16_15, true)) ;
	addTestResult (genericTestIsNeighbour (sv_WavPCM16_m32768, sv_WavPCM16_0, false)) ;
	addTestResult (genericTestIsNeighbour (sv_WavPCM16_m32768, sv_WavPCM16_32767, false)) ;
}

void WavPCMSampleValueTest::testNearestTarget ()
{
	// at both ends of the range the nearest value with the other embedded
	// value is one step inwards, never past the end (0.5.1 chose 256 for an
	// 8-bit 255 half of the time, which a byte stores as 0). It is chosen at
	// random when both neighbours fit, so try a few times.
	Globs = gl_WavPCM8 ;
	bool ok = true ;
	for (unsigned int i = 0 ; i < 20 ; i++) {
		ok = ok && (sv_WavPCM8_255->getNearestTargetSampleValue(0)->getKey() == 254) ;
		ok = ok && (sv_WavPCM8_0->getNearestTargetSampleValue(1)->getKey() == 1) ;
	}
	addTestResult (ok) ;

	Globs = gl_WavPCM16 ;
	ok = true ;
	for (unsigned int i = 0 ; i < 20 ; i++) {
		ok = ok && ((SWORD16) sv_WavPCM16_32767->getNearestTargetSampleValue(0)->getKey() == 32766) ;
		ok = ok && ((SWORD16) sv_WavPCM16_m32768->getNearestTargetSampleValue(1)->getKey() == -32767) ;
	}
	addTestResult (ok) ;
}
