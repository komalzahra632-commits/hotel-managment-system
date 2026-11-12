#include "HotelSystem.h"

#include <iostream>
#include <string>

namespace {

bool login(std::string& username, UserRole& role) {
    constexpr int maxAttempts = 3;
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        std::cout << "\nUsername: ";
        std::getline(std::cin, username);
        std::cout << "Password: ";
        std::string password;
        std::getline(std::cin, password);

        if (username == "admin" && password == "Admin@123") {
            role = UserRole::Admin;
            return true;
        }
        if (username == "receptionist" && password == "Reception@123") {
            role = UserRole::Receptionist;
            return true;
        }
        std::cout << "Invalid username or password. Attempts remaining: "
                  << maxAttempts - attempt << '\n';
    }
    std::cout << "Maximum login attempts reached. Exiting.\n";
    return false;
}

}  // namespace

int main() {
    std::cout << "========================================\n"
                 "       GRAND HOTEL MANAGEMENT SYSTEM\n"
                 "========================================\n"
                 "Demo accounts: admin / Admin@123\n"
                 "               receptionist / Reception@123\n"
                 "Use these demonstration credentials only; passwords are not stored securely.\n";

    std::string username;
    UserRole role{};
    if (!login(username, role)) return 1;
    std::cout << "\nLogin successful. Welcome, " << username << ".\n";

    HotelSystem hotel("data");
    if (!hotel.load()) {
        std::cerr << "The saved hotel records could not be loaded. Exiting to protect the data.\n";
        return 1;
    }
    hotel.run(role);
    return 0;
}
