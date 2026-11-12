#include "HotelSystem.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

constexpr double kMoneyTolerance = 0.005;

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

int readInt(const std::string& prompt) {
    while (true) {
        const std::string input = readLine(prompt);
        std::istringstream stream(input);
        int value{};
        char extra{};
        if (stream >> value && !(stream >> extra)) {
            return value;
        }
        std::cout << "Enter a valid whole number.\n";
    }
}

double readMoney(const std::string& prompt, bool allowZero = false) {
    while (true) {
        const std::string input = readLine(prompt);
        std::istringstream stream(input);
        double value{};
        char extra{};
        if ((stream >> value) && !(stream >> extra) && std::isfinite(value) &&
            (allowZero ? value >= 0.0 : value > 0.0)) {
            return value;
        }
        std::cout << (allowZero ? "Enter a valid non-negative amount.\n"
                                : "Enter a valid amount greater than zero.\n");
    }
}

bool isLeapYear(int year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

bool parseDate(const std::string& date, int& year, int& month, int& day) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        return false;
    }
    for (std::size_t index = 0; index < date.size(); ++index) {
        if (index != 4 && index != 7 &&
            !std::isdigit(static_cast<unsigned char>(date[index]))) {
            return false;
        }
    }
    try {
        year = std::stoi(date.substr(0, 4));
        month = std::stoi(date.substr(5, 2));
        day = std::stoi(date.substr(8, 2));
    } catch (const std::exception&) {
        return false;
    }
    if (year < 1 || month < 1 || month > 12) {
        return false;
    }
    static constexpr int daysInMonth[] = {31, 28, 31, 30, 31, 30,
                                          31, 31, 30, 31, 30, 31};
    const int maxDay = daysInMonth[month - 1] + (month == 2 && isLeapYear(year) ? 1 : 0);
    return day >= 1 && day <= maxDay;
}

long long daysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
    const int adjustedMonth = static_cast<int>(month) + (month > 2 ? -3 : 9);
    const unsigned dayOfYear = static_cast<unsigned>(
        (153 * adjustedMonth + 2) / 5 + static_cast<int>(day) - 1);
    const unsigned dayOfEra =
        yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return static_cast<long long>(era) * 146097 + static_cast<long long>(dayOfEra);
}

bool validDate(const std::string& date) {
    int year{};
    int month{};
    int day{};
    return parseDate(date, year, month, day);
}

std::string todayDate() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif
    std::ostringstream result;
    result << std::put_time(&localTime, "%Y-%m-%d");
    return result.str();
}

std::string readValidDate(const std::string& prompt) {
    while (true) {
        const std::string date = readLine(prompt);
        if (validDate(date)) {
            return date;
        }
        std::cout << "Enter a real date in YYYY-MM-DD format.\n";
    }
}

template <typename T, typename Reader>
bool loadRecords(const std::filesystem::path& path, std::vector<T>& records, Reader reader) {
    std::ifstream file(path);
    if (!file) {
        if (!std::filesystem::exists(path)) {
            return true;
        }
        std::cerr << "Could not open " << path.string() << " for reading.\n";
        return false;
    }
    try {
        std::string line;
        std::size_t lineNumber = 0;
        while (std::getline(file, line)) {
            ++lineNumber;
            if (line.empty()) {
                throw std::runtime_error("Empty record at " + path.string() + ":" +
                                         std::to_string(lineNumber));
            }
            T record{};
            std::istringstream stream(line);
            if (!reader(stream, record) || stream.peek() != std::char_traits<char>::eof()) {
                throw std::runtime_error("Invalid record at " + path.string() + ":" +
                                         std::to_string(lineNumber));
            }
            records.push_back(std::move(record));
        }
        if (!file.eof()) {
            throw std::runtime_error("Read failure in " + path.string());
        }
    } catch (const std::exception& error) {
        std::cerr << "Data loading error: " << error.what() << '\n';
        return false;
    }
    return true;
}

template <typename T, typename Writer>
bool saveRecords(const std::filesystem::path& path, const std::vector<T>& records,
                 Writer writer) {
    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        std::cerr << "Could not open " << path.string() << " for writing.\n";
        return false;
    }
    for (const T& record : records) {
        writer(file, record);
        file << '\n';
        if (!file) {
            std::cerr << "Failed while writing " << path.string() << ".\n";
            return false;
        }
    }
    return true;
}

std::string statusLabel(const std::string& status) {
    if (status == "Booked") return "Booked";
    if (status == "CheckedIn") return "Checked in";
    if (status == "CheckedOut") return "Checked out";
    return "Cancelled";
}

long long reservationNights(const Reservation& reservation) {
    int startYear{}, startMonth{}, startDay{};
    int endYear{}, endMonth{}, endDay{};
    parseDate(reservation.checkIn, startYear, startMonth, startDay);
    parseDate(reservation.checkOut, endYear, endMonth, endDay);
    return daysFromCivil(endYear, static_cast<unsigned>(endMonth),
                         static_cast<unsigned>(endDay)) -
           daysFromCivil(startYear, static_cast<unsigned>(startMonth),
                         static_cast<unsigned>(startDay));
}

void printMoney(double amount) {
    std::cout << "Rs. " << std::fixed << std::setprecision(2) << amount
              << std::defaultfloat;
}

}  // namespace

HotelSystem::HotelSystem(std::filesystem::path dataDirectory)
    : dataDirectory_(std::move(dataDirectory)) {}

bool HotelSystem::load() {
    try {
        std::filesystem::create_directories(dataDirectory_);
    } catch (const std::filesystem::filesystem_error& error) {
        std::cerr << "Could not create data directory: " << error.what() << '\n';
        return false;
    }

    const auto roomsPath = dataDirectory_ / "rooms.txt";
    if (!loadRecords(roomsPath, rooms_, [](std::istream& in, Room& room) {
            return static_cast<bool>(in >> room.id >> std::quoted(room.type) >> room.pricePerNight) &&
                   room.id > 0 && !room.type.empty() && std::isfinite(room.pricePerNight) &&
                   room.pricePerNight > 0.0;
        }) ||
        !loadRecords(dataDirectory_ / "guests.txt", guests_,
                     [](std::istream& in, Guest& guest) {
                         return static_cast<bool>(in >> guest.id >> std::quoted(guest.name) >>
                                                  std::quoted(guest.phone) >>
                                                  std::quoted(guest.email)) &&
                                guest.id > 0 && !guest.name.empty();
                     }) ||
        !loadRecords(dataDirectory_ / "reservations.txt", reservations_,
                     [](std::istream& in, Reservation& reservation) {
                         return static_cast<bool>(
                                    in >> reservation.id >> reservation.guestId >>
                                    reservation.roomId >> std::quoted(reservation.checkIn) >>
                                    std::quoted(reservation.checkOut) >>
                                    std::quoted(reservation.status) >> reservation.pricePerNight >>
                                    reservation.serviceCharges >> reservation.discount) &&
                                reservation.id > 0 && reservation.guestId > 0 &&
                                reservation.roomId > 0 && validDate(reservation.checkIn) &&
                                validDate(reservation.checkOut) &&
                                reservation.checkOut > reservation.checkIn &&
                                (reservation.status == "Booked" ||
                                 reservation.status == "CheckedIn" ||
                                 reservation.status == "CheckedOut" ||
                                 reservation.status == "Cancelled") &&
                                std::isfinite(reservation.pricePerNight) &&
                                reservation.pricePerNight > 0.0 &&
                                std::isfinite(reservation.serviceCharges) &&
                                reservation.serviceCharges >= 0.0 &&
                                std::isfinite(reservation.discount) &&
                                reservation.discount >= 0.0;
                     }) ||
        !loadRecords(dataDirectory_ / "payments.txt", payments_,
                     [](std::istream& in, Payment& payment) {
                         return static_cast<bool>(in >> payment.id >> payment.reservationId >>
                                                  payment.amount >> std::quoted(payment.date)) &&
                                payment.id > 0 && payment.reservationId > 0 &&
                                std::isfinite(payment.amount) && payment.amount > 0.0 &&
                                validDate(payment.date);
                     })) {
        return false;
    }

    if (rooms_.empty()) {
        rooms_ = {{101, "Standard", 5000.0},
                  {102, "Deluxe", 8000.0},
                  {201, "Executive", 12000.0},
                  {301, "Suite", 20000.0}};
        std::cout << "Created four sample rooms. Use Manage Rooms to add or update rooms.\n";
    }
    return true;
}

bool HotelSystem::save() const {
    try {
        std::filesystem::create_directories(dataDirectory_);
    } catch (const std::filesystem::filesystem_error& error) {
        std::cerr << "Could not create data directory: " << error.what() << '\n';
        return false;
    }
    const bool roomsSaved = saveRecords(dataDirectory_ / "rooms.txt", rooms_,
                                        [](std::ostream& out, const Room& room) {
                                            out << room.id << ' ' << std::quoted(room.type) << ' '
                                                << std::setprecision(17) << room.pricePerNight;
                                        });
    const bool guestsSaved = saveRecords(dataDirectory_ / "guests.txt", guests_,
                                         [](std::ostream& out, const Guest& guest) {
                                             out << guest.id << ' ' << std::quoted(guest.name)
                                                 << ' ' << std::quoted(guest.phone) << ' '
                                                 << std::quoted(guest.email);
                                         });
    const bool reservationsSaved =
        saveRecords(dataDirectory_ / "reservations.txt", reservations_,
                    [](std::ostream& out, const Reservation& reservation) {
                        out << reservation.id << ' ' << reservation.guestId << ' '
                            << reservation.roomId << ' ' << std::quoted(reservation.checkIn) << ' '
                            << std::quoted(reservation.checkOut) << ' '
                            << std::quoted(reservation.status) << ' ' << std::setprecision(17)
                            << reservation.pricePerNight << ' ' << reservation.serviceCharges
                            << ' ' << reservation.discount;
                    });
    const bool paymentsSaved = saveRecords(dataDirectory_ / "payments.txt", payments_,
                                           [](std::ostream& out, const Payment& payment) {
                                               out << payment.id << ' ' << payment.reservationId
                                                   << ' ' << std::setprecision(17) << payment.amount
                                                   << ' ' << std::quoted(payment.date);
                                           });
    return roomsSaved && guestsSaved && reservationsSaved && paymentsSaved;
}

void HotelSystem::run(UserRole role) {
    while (true) {
        std::cout << "\n========================================\n"
                  << (role == UserRole::Admin ? "           ADMIN DASHBOARD\n"
                                              : "        RECEPTIONIST DASHBOARD\n")
                  << "========================================\n"
                  << "1. Dashboard statistics\n"
                  << "2. Manage rooms\n"
                  << "3. Register new guest\n"
                  << "4. Book a room\n"
                  << "5. View all reservations\n"
                  << "6. Search reservations\n"
                  << "7. Cancel reservation\n"
                  << "8. Check in guest\n"
                  << "9. Check out guest\n"
                  << "10. Generate bill / record payment\n"
                  << "11. View revenue report\n"
                  << "12. Save records\n"
                  << "13. Logout\n";
        const int choice = readInt("Enter your choice: ");
        switch (choice) {
            case 1: showDashboard(); break;
            case 2:
                if (role == UserRole::Admin) manageRooms();
                else std::cout << "Only an admin can manage rooms.\n";
                break;
            case 3: registerGuest(); break;
            case 4: bookRoom(); break;
            case 5: viewReservations(); break;
            case 6: searchReservations(); break;
            case 7: cancelReservation(); break;
            case 8: checkInGuest(); break;
            case 9: checkOutGuest(); break;
            case 10: manageBill(); break;
            case 11:
                if (role == UserRole::Admin) showRevenueReport();
                else std::cout << "Only an admin can view the revenue report.\n";
                break;
            case 12: saveWithMessage(); break;
            case 13:
                saveWithMessage();
                std::cout << "Logged out.\n";
                return;
            default: std::cout << "Choose an option from 1 to 13.\n"; break;
        }
    }
}

Room* HotelSystem::findRoom(int id) {
    const auto it = std::find_if(rooms_.begin(), rooms_.end(),
                                 [id](const Room& room) { return room.id == id; });
    return it == rooms_.end() ? nullptr : &*it;
}

const Room* HotelSystem::findRoom(int id) const {
    const auto it = std::find_if(rooms_.begin(), rooms_.end(),
                                 [id](const Room& room) { return room.id == id; });
    return it == rooms_.end() ? nullptr : &*it;
}

Guest* HotelSystem::findGuest(int id) {
    const auto it = std::find_if(guests_.begin(), guests_.end(),
                                 [id](const Guest& guest) { return guest.id == id; });
    return it == guests_.end() ? nullptr : &*it;
}

const Guest* HotelSystem::findGuest(int id) const {
    const auto it = std::find_if(guests_.begin(), guests_.end(),
                                 [id](const Guest& guest) { return guest.id == id; });
    return it == guests_.end() ? nullptr : &*it;
}

Reservation* HotelSystem::findReservation(int id) {
    const auto it = std::find_if(reservations_.begin(), reservations_.end(),
                                 [id](const Reservation& reservation) {
                                     return reservation.id == id;
                                 });
    return it == reservations_.end() ? nullptr : &*it;
}

const Reservation* HotelSystem::findReservation(int id) const {
    const auto it = std::find_if(reservations_.begin(), reservations_.end(),
                                 [id](const Reservation& reservation) {
                                     return reservation.id == id;
                                 });
    return it == reservations_.end() ? nullptr : &*it;
}

double HotelSystem::amountPaid(int reservationId) const {
    double total = 0.0;
    for (const Payment& payment : payments_) {
        if (payment.reservationId == reservationId) {
            total += payment.amount;
        }
    }
    return total;
}

double HotelSystem::billTotal(const Reservation& reservation) const {
    return std::max(0.0, static_cast<double>(reservationNights(reservation)) *
                             reservation.pricePerNight +
                             reservation.serviceCharges - reservation.discount);
}

bool HotelSystem::roomAvailable(int roomId, const std::string& checkIn,
                                const std::string& checkOut, int ignoredReservationId) const {
    for (const Reservation& reservation : reservations_) {
        if (reservation.id == ignoredReservationId || reservation.roomId != roomId ||
            (reservation.status != "Booked" && reservation.status != "CheckedIn")) {
            continue;
        }
        if (checkIn < reservation.checkOut && reservation.checkIn < checkOut) {
            return false;
        }
    }
    return true;
}

void HotelSystem::showDashboard() const {
    int occupied = 0;
    for (const Room& room : rooms_) {
        const bool isOccupied = std::any_of(
            reservations_.begin(), reservations_.end(), [&room](const Reservation& reservation) {
                return reservation.roomId == room.id && reservation.status == "CheckedIn";
            });
        occupied += isOccupied ? 1 : 0;
    }
    double revenue = 0.0;
    for (const Payment& payment : payments_) revenue += payment.amount;
    std::cout << "\n--- Hotel Statistics ---\n"
              << "Total rooms: " << rooms_.size() << "\n"
              << "Occupied rooms: " << occupied << "\n"
              << "Available rooms: " << rooms_.size() - static_cast<std::size_t>(occupied)
              << "\n"
              << "Registered guests: " << guests_.size() << "\n"
              << "Recorded revenue: ";
    printMoney(revenue);
    std::cout << '\n';
}

void HotelSystem::listRooms() const {
    if (rooms_.empty()) {
        std::cout << "No rooms have been registered.\n";
        return;
    }
    std::cout << "\nRoom ID | Type | Rate per night | Status\n";
    for (const Room& room : rooms_) {
        const bool occupied = std::any_of(
            reservations_.begin(), reservations_.end(), [&room](const Reservation& reservation) {
                return reservation.roomId == room.id && reservation.status == "CheckedIn";
            });
        std::cout << room.id << " | " << room.type << " | ";
        printMoney(room.pricePerNight);
        std::cout << " | " << (occupied ? "Occupied" : "Available") << '\n';
    }
}

void HotelSystem::manageRooms() {
    while (true) {
        std::cout << "\n--- Room Management ---\n1. List rooms\n2. Add room\n"
                     "3. Update room\n4. Return to dashboard\n";
        const int choice = readInt("Enter your choice: ");
        if (choice == 1) {
            listRooms();
        } else if (choice == 2) {
            const int id = readInt("Room number: ");
            if (id <= 0 || findRoom(id) != nullptr) {
                std::cout << "Room number must be positive and unused.\n";
                continue;
            }
            std::string type = readLine("Room type: ");
            if (type.empty()) {
                std::cout << "Room type cannot be empty.\n";
                continue;
            }
            const double price = readMoney("Price per night (Rs.): ");
            rooms_.push_back({id, type, price});
            saveWithMessage();
            std::cout << "Room added.\n";
        } else if (choice == 3) {
            const int id = readInt("Room number to update: ");
            Room* room = findRoom(id);
            if (room == nullptr) {
                std::cout << "Room not found.\n";
                continue;
            }
            const std::string type = readLine("New room type (blank keeps current): ");
            const std::string priceInput =
                readLine("New price per night (blank keeps current): ");
            double price = room->pricePerNight;
            if (!priceInput.empty()) {
                std::istringstream stream(priceInput);
                char extra{};
                if (!(stream >> price) || (stream >> extra) || !std::isfinite(price) ||
                    price <= 0.0) {
                    std::cout << "Invalid price; room was not updated.\n";
                    continue;
                }
            }
            if (!type.empty()) room->type = type;
            room->pricePerNight = price;
            saveWithMessage();
            std::cout << "Room updated.\n";
        } else if (choice == 4) {
            return;
        } else {
            std::cout << "Choose an option from 1 to 4.\n";
        }
    }
}

void HotelSystem::registerGuest() {
    Guest guest;
    guest.id = 1;
    for (const Guest& existing : guests_) guest.id = std::max(guest.id, existing.id + 1);
    guest.name = readLine("Guest name: ");
    if (guest.name.empty()) {
        std::cout << "Guest name cannot be empty.\n";
        return;
    }
    guest.phone = readLine("Phone number: ");
    guest.email = readLine("Email address: ");
    guests_.push_back(guest);
    saveWithMessage();
    std::cout << "Guest registered. Guest ID: " << guest.id << '\n';
}

void HotelSystem::listGuests() const {
    if (guests_.empty()) {
        std::cout << "No guests have been registered.\n";
        return;
    }
    std::cout << "\nGuest ID | Name | Phone | Email\n";
    for (const Guest& guest : guests_) {
        std::cout << guest.id << " | " << guest.name << " | " << guest.phone << " | "
                  << guest.email << '\n';
    }
}

void HotelSystem::bookRoom() {
    if (guests_.empty() || rooms_.empty()) {
        std::cout << "Register a guest and add a room before making a reservation.\n";
        return;
    }
    listGuests();
    const int guestId = readInt("Guest ID: ");
    if (findGuest(guestId) == nullptr) {
        std::cout << "Guest not found.\n";
        return;
    }
    listRooms();
    const int roomId = readInt("Room number: ");
    if (findRoom(roomId) == nullptr) {
        std::cout << "Room not found.\n";
        return;
    }
    const std::string checkIn = readValidDate("Check-in date (YYYY-MM-DD): ");
    const std::string checkOut = readValidDate("Check-out date (YYYY-MM-DD): ");
    if (checkOut <= checkIn) {
        std::cout << "Check-out must be later than check-in.\n";
        return;
    }
    if (!roomAvailable(roomId, checkIn, checkOut)) {
        std::cout << "That room is already reserved during some or all of those dates.\n";
        return;
    }
    int id = 1;
    for (const Reservation& existing : reservations_) id = std::max(id, existing.id + 1);
    reservations_.push_back(
        {id, guestId, roomId, checkIn, checkOut, "Booked", findRoom(roomId)->pricePerNight,
         0.0, 0.0});
    saveWithMessage();
    std::cout << "Reservation created. Reservation ID: " << id << '\n';
}

void HotelSystem::viewReservations() const {
    if (reservations_.empty()) {
        std::cout << "No reservations found.\n";
        return;
    }
    std::cout << "\nID | Guest | Room | Check-in | Check-out | Status\n";
    for (const Reservation& reservation : reservations_) {
        const Guest* guest = findGuest(reservation.guestId);
        std::cout << reservation.id << " | " << (guest ? guest->name : "Unknown") << " | "
                  << reservation.roomId << " | " << reservation.checkIn << " | "
                  << reservation.checkOut << " | " << statusLabel(reservation.status) << '\n';
    }
}

void HotelSystem::searchReservations() const {
    const std::string query = readLine("Enter reservation ID or guest name: ");
    if (query.empty()) {
        std::cout << "Search value cannot be empty.\n";
        return;
    }
    bool found = false;
    for (const Reservation& reservation : reservations_) {
        const Guest* guest = findGuest(reservation.guestId);
        bool matches = std::to_string(reservation.id) == query;
        if (guest != nullptr && guest->name.find(query) != std::string::npos) matches = true;
        if (matches) {
            found = true;
            std::cout << "Reservation " << reservation.id << " | "
                      << (guest ? guest->name : "Unknown") << " | Room " << reservation.roomId
                      << " | " << reservation.checkIn << " to " << reservation.checkOut << " | "
                      << statusLabel(reservation.status) << '\n';
        }
    }
    if (!found) std::cout << "No matching reservations found.\n";
}

void HotelSystem::cancelReservation() {
    const int id = readInt("Reservation ID to cancel: ");
    Reservation* reservation = findReservation(id);
    if (reservation == nullptr || reservation->status != "Booked") {
        std::cout << "Only a booked reservation can be cancelled.\n";
        return;
    }
    if (amountPaid(id) > kMoneyTolerance) {
        std::cout << "This reservation has recorded payments; resolve the payment before "
                     "cancelling it.\n";
        return;
    }
    reservation->status = "Cancelled";
    saveWithMessage();
    std::cout << "Reservation cancelled. Its room is available for those dates again.\n";
}

void HotelSystem::checkInGuest() {
    const int id = readInt("Reservation ID to check in: ");
    Reservation* reservation = findReservation(id);
    if (reservation == nullptr || reservation->status != "Booked") {
        std::cout << "No booked reservation found with that ID.\n";
        return;
    }
    const std::string today = todayDate();
    if (today < reservation->checkIn || today >= reservation->checkOut) {
        std::cout << "Check-in is only allowed from the reservation check-in date up to "
                     "the day before check-out.\n";
        return;
    }
    reservation->status = "CheckedIn";
    saveWithMessage();
    std::cout << "Guest checked in.\n";
}

void HotelSystem::checkOutGuest() {
    const int id = readInt("Reservation ID to check out: ");
    Reservation* reservation = findReservation(id);
    if (reservation == nullptr || reservation->status != "CheckedIn") {
        std::cout << "No checked-in reservation found with that ID.\n";
        return;
    }
    const double balance = std::max(0.0, billTotal(*reservation) - amountPaid(id));
    std::cout << "Outstanding balance: ";
    printMoney(balance);
    std::cout << '\n';
    if (balance > kMoneyTolerance) {
        std::cout << "Settle the bill using option 10 before checking out.\n";
        return;
    }
    reservation->status = "CheckedOut";
    saveWithMessage();
    std::cout << "Guest checked out.\n";
}

void HotelSystem::manageBill() {
    const int id = readInt("Reservation ID: ");
    Reservation* reservation = findReservation(id);
    if (reservation == nullptr || reservation->status == "Cancelled") {
        std::cout << "Active reservation not found.\n";
        return;
    }
    const Room* room = findRoom(reservation->roomId);
    const Guest* guest = findGuest(reservation->guestId);
    if (room == nullptr) {
        std::cout << "The reservation's room record is missing; billing cannot continue.\n";
        return;
    }
    if (reservation->status != "CheckedOut") {
        const double additionalCharges =
            readMoney("Additional service/food charges (Rs., 0 if none): ", true);
        const double discount = readMoney("Discount to apply (Rs., 0 if none): ", true);
        const double subtotal =
            static_cast<double>(reservationNights(*reservation)) * reservation->pricePerNight +
            reservation->serviceCharges + additionalCharges;
        if (discount > subtotal ||
            amountPaid(id) > subtotal - discount + kMoneyTolerance) {
            std::cout << "Discount is invalid or would make recorded payments exceed the bill; "
                         "bill was not changed.\n";
            return;
        }
        reservation->serviceCharges += additionalCharges;
        reservation->discount = discount;
    }
    const double total = billTotal(*reservation);
    const double paid = amountPaid(id);
    const double balance = std::max(0.0, total - paid);
    const long long nights = reservationNights(*reservation);
    std::cout << "\n========== HOTEL RECEIPT ==========\n"
              << "Reservation: " << reservation->id << "\n"
              << "Guest: " << (guest ? guest->name : "Unknown") << "\n"
              << "Room: " << room->id << " (" << room->type << ")\n"
              << "Stay: " << reservation->checkIn << " to " << reservation->checkOut
              << " (" << nights << " nights)\n"
              << "Room charges: ";
    printMoney(static_cast<double>(nights) * reservation->pricePerNight);
    std::cout << "\nService/food charges: ";
    printMoney(reservation->serviceCharges);
    std::cout << "\nDiscount: ";
    printMoney(reservation->discount);
    std::cout << "\nTotal: ";
    printMoney(total);
    std::cout << "\nPaid: ";
    printMoney(paid);
    std::cout << "\nBalance: ";
    printMoney(balance);
    std::cout << "\n===================================\n";

    if (reservation->status != "CheckedOut" && balance > kMoneyTolerance) {
        const double paymentAmount =
            readMoney("Payment to record (Rs., 0 to skip): ", true);
        if (paymentAmount > balance + kMoneyTolerance) {
            std::cout << "Payment exceeds the outstanding balance; no payment recorded.\n";
            saveWithMessage();
            return;
        }
        if (paymentAmount > 0.0) {
            int paymentId = 1;
            for (const Payment& payment : payments_) {
                paymentId = std::max(paymentId, payment.id + 1);
            }
            payments_.push_back({paymentId, id, paymentAmount, todayDate()});
            std::cout << "Payment recorded. Payment ID: " << paymentId << '\n';
        }
    }
    saveWithMessage();
}

void HotelSystem::showRevenueReport() const {
    const std::string date = readValidDate("Revenue date (YYYY-MM-DD): ");
    double revenue = 0.0;
    int count = 0;
    std::cout << "\nPayments recorded on " << date << ":\n";
    for (const Payment& payment : payments_) {
        if (payment.date == date) {
            std::cout << "Payment " << payment.id << " | Reservation "
                      << payment.reservationId << " | ";
            printMoney(payment.amount);
            std::cout << '\n';
            revenue += payment.amount;
            ++count;
        }
    }
    std::cout << "Transactions: " << count << "\nDaily revenue: ";
    printMoney(revenue);
    std::cout << '\n';
}

void HotelSystem::saveWithMessage() const {
    if (save()) {
        std::cout << "Records saved.\n";
    } else {
        std::cerr << "One or more records could not be saved. Check the data directory and disk.\n";
    }
}
