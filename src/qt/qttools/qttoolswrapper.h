/*****************************************************************************
 * Raise73 - Texas Holdem No Limit software, offline game against custom AIs *
 * Copyright (C) 2024 Marc Ennaji                                            *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *****************************************************************************/
#ifndef QTTOOLSWRAPPER_H
#define QTTOOLSWRAPPER_H

#include <qt/qttoolsinterface.h>

#include <string>

class QtHelper;

class QtToolsWrapper : public QtToolsInterface
{
public:
	QtToolsWrapper();

	~QtToolsWrapper();

	std::string stringToUtf8(const std::string &myString);
	std::string stringFromUtf8(const std::string &myString);
	std::string getDefaultLanguage();
	std::string getDataPathStdString(const char *argv0);

private:

	QtHelper *myQtHelper;

};

#endif
