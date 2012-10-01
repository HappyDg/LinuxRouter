#ifndef PPP_H
#define PPP_H

/** @file ppp.h
 *  @brief Point to point protocol. 

    Copyright 2007-2008 j. Arzi.

    This file is part of SDPOS.

    SDPOS is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SDPOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with SDPOS.  If not, see <http://www.gnu.org/licenses/>.
 **/


/** @brief Is 's' a valid PPP frame? */
extern bool ppp_validate(unsigned char *s, unsigned char len);

/** @brief Set streams to use by PPP. */
extern void ppp_set_iostream(iostream_t ios);

/** @brief Close the PPP link. */
extern void ppp_close(void);

#endif /* PPP_H */
