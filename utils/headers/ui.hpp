#pragma once

#include "action.hpp"

#include <string>
#include <vector>

#define PREFIX "$"

using namespace std;

void showMenu ();
void showHelp();
vector<string> parseUserCommand(const string& input, const string& separator);
ClientAction* getUserCommand();
