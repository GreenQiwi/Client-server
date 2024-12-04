#pragma once
#include <iostream>

class Authentication {
public:
	Authentication();
	void authenticate();

	std::string login;
	std::string password;
};