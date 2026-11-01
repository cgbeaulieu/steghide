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

#include <string>
#include <vector>

#include "common.h"
#include "MHashKeyGen.h"
#include "MHashPP.h"

MHashKeyGen::MHashKeyGen (unsigned int keysize)
	: KeySize(keysize)
{
}

std::vector<BYTE> MHashKeyGen::createKey (std::string password)
{
	// steghide 0.5.1 passed the password to libmhash with strcpy and strlen,
	// so a password with a '\0' in it was cut off there
	std::string pw (password.c_str()) ;

	std::vector<BYTE> key ;
	while (key.size() < KeySize) {
		MHashPP hash (MHashPP::MD5) ;
		hash << pw ;
		for (std::vector<BYTE>::const_iterator i = key.begin() ; i != key.end() ; i++) {
			hash << *i ;
		}
		hash << MHashPP::endhash ;
		const std::vector<BYTE>& digest = hash.getHashBytes() ;
		for (std::vector<BYTE>::const_iterator i = digest.begin() ; i != digest.end() && key.size() < KeySize ; i++) {
			key.push_back (*i) ;
		}
	}
	return key ;
}
