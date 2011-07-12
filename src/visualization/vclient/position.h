/* Copyright (c) 2008 Uppsala Universitet.
 * All rights reserved.
 * 
 * This file is part of Vendetta.
 *
 * Vendetta is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Vendetta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Vendetta.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef POSITIONH
#define POSITIONH

#define POSITION_FIFO "run/position"

int position_create_fifo();
int position_delete_fifo();
int position_open_fifo();
void position_read_fifo();
int position_read_from_file(const char *filename);
int position_write_to_file(const char *filename);

#endif /* POSITIONH */

/* vim:set sw=2 ts=8 expandtab: */