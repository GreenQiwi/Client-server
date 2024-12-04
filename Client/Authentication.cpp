#include "Authentication.hpp"

Authentication::Authentication() :
	login("default"), password("default") {}

void Authentication::authenticate()
{
	std::cout << "Enter login: ";
	std::cin >> login;
	std::cout << "Enter password: ";
	std::cin >> password;
	std::cout << std::endl;
}
