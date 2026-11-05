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

#include <vector>

#include "common.h"
#include "RandomSource.h"

#include "RandomSourceTest.h"
#include "TestCategoryCaller.h"

RandomSourceTest::RandomSourceTest (TestSuite* s)
	: UnitTest ("RandomSource", s)
{
	ADDTESTCATEGORY (RandomSourceTest, testGetBytes) ;
	ADDTESTCATEGORY (RandomSourceTest, testInstances) ;
	ADDTESTCATEGORY (RandomSourceTest, testGetValue) ;
}

void RandomSourceTest::testGetBytes ()
{
	// more than one buffer full; with random data, a byte value is missing
	// from 8192 bytes with a probability of about 10^-11
	RandomSource rs ;
	std::vector<BYTE> bytes = rs.getBytes (8192) ;
#ifdef NORANDOM
	addTestResult (bytes == std::vector<BYTE> (bytes.size(), 0)) ;
#else
	std::vector<bool> seen (256, false) ;
	for (BYTE b : bytes) {
		seen[b] = true ;
	}
	bool allseen = true ;
	for (bool s : seen) {
		allseen = allseen && s ;
	}
	addTestResult (allseen) ;
#endif
}

void RandomSourceTest::testInstances ()
{
	// two sources must not start from the same state, as they would with a fixed seed
	RandomSource rs1, rs2 ;
#ifdef NORANDOM
	addTestResult (rs1.getBytes (32) == rs2.getBytes (32)) ;
#else
	addTestResult (rs1.getBytes (32) != rs2.getBytes (32)) ;
#endif
}

void RandomSourceTest::testGetValue ()
{
	RandomSource rs ;
	std::vector<bool> seen (3, false) ;
	bool inrange = true ;
	for (unsigned int i = 0 ; i < 1000 ; i++) {
		unsigned long v = rs.getValue (3) ;
		if (v < 3) {
			seen[v] = true ;
		}
		else {
			inrange = false ;
		}
	}
	addTestResult (inrange) ;
#ifndef NORANDOM
	addTestResult (seen[0] && seen[1] && seen[2]) ;
#endif
}
