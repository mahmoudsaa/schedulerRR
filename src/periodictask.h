/*
*  Copyright (C) 2010 Simone Gaiarin <simgunz@gmail.com>
*
*  This file is part of Foobar.
*
*  Foobar is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  Foobar is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with Foobar.  If not, see <http://www.gnu.org/licenses/>. 
*/

#ifndef PERIODICTASK_H
#define PERIODICTASK_H

#include "task.h"

class PeriodicTask : public Task
{
public:
    PeriodicTask(string fileName, float period);
    float getPeriod();
private:
    float p;
};

#endif // PERIODICTASK_H